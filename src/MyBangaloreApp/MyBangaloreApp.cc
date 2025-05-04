// MyNewProject/src/bangalore_v2v/MyBangaloreApp.cc

// Include your custom application header FIRST
#include "MyBangaloreApp.h"  // Updated to use relative path
// Include other necessary Veins headers (already in .h)
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/base/utils/SimpleAddress.h"
#include "veins/base/modules/BaseMobility.h" // Keep for now if needed by DemoBaseApplLayer

// Include headers for message types you handle or send (already in .h)
#include "veins/modules/messages/BaseFrame1609_4_m.h"
#include "veins/modules/messages/DemoSafetyMessage_m.h"
#include "veins/modules/messages/DemoServiceAdvertisement_m.h"

// Include standard C++ headers
#include <fstream>
#include <sstream>

// Include cPacket header for casting
#include "omnetpp/cpacket.h"

// Use namespaces
using namespace omnetpp;
using namespace veins;
using namespace bangalore_v2v; // <-- Changed from myproject to bangalore_v2v

// Register your custom application module
Define_Module(bangalore_v2v::MyBangaloreApp); // <-- Updated with correct namespace


void MyBangaloreApp::initialize(int stage) {
    // Always call the base class initialize first
    DemoBaseApplLayer::initialize(stage);

    // Move your custom initialization logic to a later stage
    // Stage 2 might be needed if mobility details are set later
    if (stage == 1) {
        // Your initialization logic (broadcastInterval, timer, logFile) goes here
        broadcastInterval = par("broadcastInterval").doubleValueInUnit("s");
        broadcastTimer = new cMessage("v2v_broadcast_timer");

        logFile.open("v2v_log.txt", std::ios::app);
        if (!logFile.is_open()) {
            EV_ERROR << "Could not open log file v2v_log.txt for node " << getParentModule()->getFullName() << endl;
        } else {
             logFile << "Simulation started. Node: " << getParentModule()->getFullName() << std::endl;
        }

        // Schedule the first broadcast in stage 2
        scheduleAt(simTime() + uniform(0.0, broadcastInterval), broadcastTimer);
    }

    // If you had logic for other stages, keep them in their respective if blocks
    // e.g., if (stage == 3) { ... }
}

void MyBangaloreApp::handleSelfMsg(cMessage* msg) {
    if (msg->getArrivalGate() == lowerGate) { // Check if message is from the network
           // This is an incoming V2V message
           // You might need to cast it to your specific V2V message type
           // Example assuming a V2VMessage type with methods like getPosition() and getSpeed()
           // If you haven't defined a custom message type yet, you might just log cMessage info.

           // Assuming message type is V2VMessage (replace with your actual message type if different)
           // V2VMessage* v2vMsg = dynamic_cast<V2VMessage*>(msg);

           // Check if the log file is open before writing
           if (logFile.is_open()) {
               // Write to the log file in the desired format
               // Example format: [time] Sender -> Receiver : "Content"
               logFile << "[" << simTime() << "s] "
                       << msg->getSenderModule()->getName() << " -> " // Sender module name (e.g., node[x].appl)
                       << getParentModule()->getName()      // Receiver module name (e.g., node[y].appl)
                       << " : \"" << msg->getName() << "\""; // Message name or content

               // If you have a custom message type, you can access its fields:
               // if (v2vMsg) {
               //     logFile << " Position: (" << v2vMsg->getPosition().x << "," << v2vMsg->getPosition().y << ")"
               //             << ", Speed: " << v2vMsg->getSpeed() << " m/s";
               // }

               logFile << "\n"; // Add a newline after each log entry
               logFile.flush(); // Ensure data is written to file immediately (optional but good for debugging)
           }

           // Delete the message after processing
           delete msg;

       }
    if (msg == broadcastTimer) {
        sendBroadcast();
        scheduleAt(simTime() + broadcastInterval, broadcastTimer);
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}


void MyBangaloreApp::sendBroadcast() {
    // Correct member name: curPosition
    Coord pos = this->curPosition;

    // Note: Getting current speed and road ID might require a different API in your version.
    // For now, we'll just use the position.
    // double speed = this->mobility->getSpeed(); // API might be different
    // std::string roadId = this->mobility->getRoadId(); // API might be different

    std::stringstream ss;
    ss << "Node: " << getParentModule()->getId()
       << ", Pos: (" << pos.x << "," << pos.y << ")";
       // Add speed and roadId if you find the correct API later
       // << ", Speed: " << speed << " m/s"
       // << ", Road: " << roadId;
    std::string msgContent = ss.str();


    // --- Sending BaseFrame1609_4 ---
    BaseFrame1609_4* wsm = new BaseFrame1609_4();
    wsm->setByteLength(50); // Example size in bytes
    wsm->setChannelNumber(180); // Use raw number for CCH


    sendDown(wsm); // Send the message

    EV_INFO << simTime() << " Node " << getParentModule()->getId() << " broadcasted a BaseFrame1609_4 message with data: \"" << msgContent << "\"" << endl;
}


// --- Message Handler Overrides ---

void MyBangaloreApp::onBSM(veins::DemoSafetyMessage* bsm) {
    // Get source node ID through a safer approach
    int senderNodeId = getParentModule()->getId(); // Default to own ID

    // Try to extract sender ID if possible - implementation depends on your Veins version
    // In newer Veins versions, you might be able to use bsm->getSenderAddress() or similar

    EV_INFO << simTime() << " Node " << getParentModule()->getId() << " received BSM from another node" << endl;

    if (logFile.is_open()) {
        logFile << "[" << simTime() << "s] Node_" << getParentModule()->getId()
                << " <-- BSM received" << std::endl;
    }

    delete bsm;
}

void MyBangaloreApp::onWSM(veins::BaseFrame1609_4* wsm) {
    // Get source info if available
    // In newer Veins versions, there might be methods like getSenderAddress() or getSenderModule()

    EV_INFO << simTime() << " Node " << getParentModule()->getId() << " received BaseFrame1609_4 message" << endl;

    // Note: To get the actual data content (pos, speed, etc.) from a received message,
    // it must have been sent using a custom message type with those fields, and you
    // would dynamic_cast the received wsm to that custom type and access its fields.

    if (logFile.is_open()) {
        logFile << "[" << simTime() << "s] Node_" << getParentModule()->getId()
                << " <-- BaseFrame1609_4 (via onWSM) received" << std::endl;
    }
    bubble("Received WSM");

    delete wsm;
}

void MyBangaloreApp::onWSA(veins::DemoServiceAdvertisment* wsa) {
    // Get source info if available

    EV_INFO << simTime() << " Node " << getParentModule()->getId() << " received WSA" << endl;

    if (logFile.is_open()) {
        logFile << "[" << simTime() << "s] Node_" << getParentModule()->getId()
                << " <-- WSA received" << std::endl;
    }

    delete wsa;
}

void MyBangaloreApp::handlePositionUpdate(cObject* obj) {
    // DemoBaseApplLayer automatically updates this->curPosition
    DemoBaseApplLayer::handlePositionUpdate(obj);

    // Add any custom logic needed after position update
}

void MyBangaloreApp::finish() {
    DemoBaseApplLayer::finish();
    if (logFile.is_open()) {
        logFile << "Simulation finished. Node: " << getParentModule()->getFullName() << std::endl;
        logFile.close();
    }
    cancelAndDelete(broadcastTimer);
}
