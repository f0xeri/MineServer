#include <iostream>

//#ifdef _WIN32
//#define _WIN32_WINNT 0x0A00
//#endif
#define ASIO_STANDALONE

#include <algorithm>
#include <MineNet.h>
#include <MineNetServer.hpp>

enum class MineMsgTypes : uint32_t
{
    ServerPing,
    ServerMessage,
    ServerMessageAll,
    ClientAccepted,
    ClientAssignID,
    ClientRegisterWithServer,
    ClientUnregisterWithServer,
    ClientGetWorldChanges,
    WorldAddPlayer,
    WorldRemovePlayer,
    WorldUpdatePlayer,
    WorldChunkModified,
    WorldChanges,
    ServerSaveWorldChange,
};

struct playerNetData
{
    uint32_t id;
    double posX, posY, posZ;
    double rotY;
    //double spawnPosX, spawnPosY, spawnPosZ;
};

struct ChunkModifyData
{
    int iendx, iendy, iendz;
    int normx, normy, normz;
    int blockId;
};

struct ChunkChangesSave
{
    int cx, cy, cz;
    int blockNumber;
    int newBlockId;
};


class MineServer : public MineNet::IServer<MineMsgTypes>
{
    std::unordered_map<uint32_t, playerNetData> playersData;
    std::vector<uint32_t> garbageIDs;
    std::vector<ChunkChangesSave> mapChangesData;
public:
    MineServer(const uint16_t port) : MineNet::IServer<MineMsgTypes>(port)
    {

    }

protected:
    bool onClientConnect(std::shared_ptr<MineNet::connection<MineMsgTypes>> client) override
    {
        return true;
    }

    void onClientValidated(std::shared_ptr<MineNet::connection<MineMsgTypes>> client) override
    {
        MineNet::message<MineMsgTypes> msg;
        msg.header.id = MineMsgTypes::ClientAccepted;
        client->send(msg);
    }

    virtual void onClientDisconnect(std::shared_ptr<MineNet::connection<MineMsgTypes>> client)
    {
        if (client)
        {
            if (playersData.find(client->getID()) == playersData.end())
            {

            }
            else
            {
                auto &pd = playersData[client->getID()];
                std::cout << "[INFO] " << pd.id << " disconnected.\n";
                playersData.erase(client->getID());
                garbageIDs.push_back(client->getID());
            }
        }
    }

    virtual void onMessage(std::shared_ptr<MineNet::connection<MineMsgTypes>> client, MineNet::message<MineMsgTypes> &msg)
    {
        if (!garbageIDs.empty())
        {
            for (auto pid : garbageIDs)
            {
                MineNet::message<MineMsgTypes> m;
                m.header.id = MineMsgTypes::WorldRemovePlayer;
                m << pid;
                std::cout << "[INFO] Removing " << pid << " id from world.\n";
                messageAllClients(m);
            }
            garbageIDs.clear();
        }

        switch (msg.header.id)
        {
            case MineMsgTypes::ClientRegisterWithServer:
            {
                playerNetData data{};
                msg >> data;
                data.id = client->getID();
                playersData.insert_or_assign(data.id, data);

                MineNet::message<MineMsgTypes> idMsg;
                idMsg.header.id = MineMsgTypes::ClientAssignID;
                idMsg << data.id;
                messageClient(client, idMsg);

                //std::cout << "add " << client->getID() << " " << client->getID() << "\n";
                MineNet::message<MineMsgTypes> addPlayerMsg;
                addPlayerMsg.header.id = MineMsgTypes::WorldAddPlayer;
                addPlayerMsg << data;
                messageAllClients(addPlayerMsg);

                for (auto &player : playersData)
                {
                    MineNet::message<MineMsgTypes> addOtherPlayersMsg;
                    addOtherPlayersMsg.header.id = MineMsgTypes::WorldAddPlayer;
                    addOtherPlayersMsg << player.second;
                    messageClient(client, addOtherPlayersMsg);
                }

                MineNet::message<MineMsgTypes> mapChangesMsg;
                mapChangesMsg.header.id = MineMsgTypes::WorldChanges;
                for (auto &change : mapChangesData)
                {
                    mapChangesMsg << change;
                    messageClient(client, mapChangesMsg);
                }

                break;
            }

            case MineMsgTypes::ClientUnregisterWithServer:
            {
                break;
            }

            case MineMsgTypes::WorldUpdatePlayer:
            {
                messageAllClients(msg, client);
                break;
            }

            case MineMsgTypes::WorldChunkModified:
            {
                messageAllClients(msg, client);
                break;
            }

            case MineMsgTypes::ServerSaveWorldChange:
            {
                ChunkChangesSave data{};
                msg >> data;
                mapChangesData.erase(
                        std::remove_if(mapChangesData.begin(), mapChangesData.end(), [&](ChunkChangesSave const &change) {
                            return change.cx == data.cx && change.cy == data.cy && change.cz == data.cz && change.blockNumber == data.blockNumber;
                        }), mapChangesData.end());
                mapChangesData.push_back(data);
                break;
            }

            case MineMsgTypes::ClientGetWorldChanges:
            {
                MineNet::message<MineMsgTypes> msg;
                msg.header.id = MineMsgTypes::WorldChanges;
                msg << mapChangesData;
                messageClient(client, msg);
                break;
            }

            case MineMsgTypes::ServerMessageAll:
            {
                std::cout << "[" << client->getID() << "]: Message All\n";

                MineNet::message<MineMsgTypes> msg;
                msg.header.id = MineMsgTypes::ServerMessage;
                msg << client->getID();
                messageAllClients(msg, client);

            }
            break;
        }
    }
};

int main()
{
    MineServer server (60000);
    server.start();
    while (1)
    {
        server.update(-1, true);
    }
    return 0;
}
