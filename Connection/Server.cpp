#include "Server.hpp"
#include "../Game/Game.hpp"
#include "../Player/Player.hpp"
#include "../Modules/Logger.hpp"

void Server::start() {
    Logger::info("Starting uWS Server...");
    connectionThread = std::thread([&]() {
        uWS::Hub hub;
        onClientConnection(&hub);
        onClientDisconnection(&hub);
        onClientMessage(&hub);

        if (hub.listen(cfg::server_port)) {
            runningState = 1;
            Logger::print("\n");
            Logger::info("Server is listening on port ", cfg::server_port);
            hub.run();
        } else {
            runningState = 0;
            Logger::error("Server couldn't listen on port ", cfg::server_port);
            Logger::error("Close out of applications running on the same port or run this with root priveleges.");
            Logger::print("Press any key to exit...\n");
        }
    });
}
void Server::onClientConnection(uWS::Hub *hub) {
    hub->onConnection([&](uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req) {
        req;
        if (++connections >= cfg::server_maxConnections) {
            ws->close(1000, "Server connection limit reached");
            return;
        }
        // allocating player because it needs to exist outside of this loop
        Player *player = new Player(ws);
        player->server = this;
        ws->setUserData(player);
        clients.push_back(player);

        Logger::debug("Connection made");
    });
}
void Server::onClientDisconnection(uWS::Hub *hub) {
    hub->onDisconnection([&](uWS::WebSocket<uWS::SERVER> *ws, int code, char *message, size_t length) {
        code;
        message;
        length;
        ((Player*)ws->getUserData())->onDisconnection();
        //ws->setUserData(nullptr);
        --connections;
        Logger::debug("Disconnection made");
    });
}
void Server::onClientMessage(uWS::Hub *hub) {
    hub->onMessage([&](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
        opCode;
        if (length == 0) return;

        if (length > 256) {
            ws->close(1009, "no spam pls");
            return;
        }
        std::vector<unsigned char> packet(message, message + length);
        Player *player = (Player*)ws->getUserData();
        if (player != nullptr)
            player->packetHandler.onPacket(packet);
    });
}
void Server::end() {
    Logger::warn("Stopping uWS Server...");
    for (Player *player : clients)
        player->socket->close();
    connectionThread.detach();
}