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
#include "veins/modules/mac/ieee80211p/Mac1609_4.h" // For channel constants
#include "veins/modules/phy/DeciderResult80211.h"
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
    EV_ERROR << "Starting something " << endl;

    // Move your custom initialization logic to a later stage
    // Stage 2 might be needed if mobility details are set later
    if (stage == 0) {

        // Your initialization logic (broadcastInterval, timer, logFile) goes here
        broadcastInterval = par("broadcastInterval").doubleValueInUnit("s");
        broadcastTimer = new cMessage("v2v_broadcast_timer");


        logFile.open("v2v_log.txt", std::ios::app);
        if (!logFile.is_open()) {
                    EV_ERROR << "Could not open log file v2v_log.txt for node " << getParentModule()->getFullName() << endl;
                    // Consider throwing an error here if logging is essential:
                    // throw cRuntimeError("Failed to open log file");
                } else {
                     EV_INFO << "Simulation started. Node: " << getParentModule()->getFullName() << ". Log file opened for appending." << endl;
                     // Write a header to the log file when opened
                     logFile << "------ Simulation Run Started at " << simTime() << " ------\n";
                     // Using tabs (\t) for separation to make it easier to parse later
                     logFile << "Time [s]\tSender ID\tReceiver ID\tMessage Info\n";
                     logFile.flush(); // Ensure header is written immediately
                }
                // --- End File Opening Code ---

                // Schedule the first broadcast in stage 2
                scheduleAt(simTime() + uniform(0.0, broadcastInterval), broadcastTimer);
    }

    // If you had logic for other stages, keep them in their respective if blocks
    // e.g., if (stage == 3) { ... }
}

void MyBangaloreApp::handleSelfMsg(cMessage* msg) {
    if (msg == broadcastTimer) {
        sendBroadcast();
        scheduleAt(simTime() + broadcastInterval, broadcastTimer);
    } else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void MyBangaloreApp::sendBroadcast() {
    // Create the message
    veins::BaseFrame1609_4* msg = new veins::BaseFrame1609_4("v2v_broadcast");

    // Get position properly using the correct method for your Veins version
    veins::Coord currentPos = mobility->getPositionAt(simTime());

    // Create content string
    std::string content = "Node: " + std::to_string(getParentModule()->getId()) +
                         ", Pos: " + currentPos.info(); // Use .info() instead of .str()

    // Create and encapsulate data
    cPacket* encapMsg = new cPacket("broadcast_data");
        encapMsg->setByteLength(content.length() + 1); // Set proper byte length

        // Store content as a parameter - fixed string conversion
        encapMsg->addPar("content").setStringValue(content.c_str());

    // Encapsulate the packet
    msg->encapsulate(encapMsg);

    // Set channel properties using proper Veins constants
    // Check the actual constants in your Veins version
    msg->setChannelNumber(static_cast<int>(veins::Channel::cch)); // Use Veins namespace and actual enum

    // Channel access might need to be set differently depending on your Veins version
    // If setChannelAccess doesn't exist, this can be skipped or adapted
    // Many Veins versions don't have this method, so you may need to remove this line
    // msg->setChannelAccess(...); // Commented out since it likely doesn't exist

        msg->addPar("posX").setDoubleValue(currentPos.x);
        msg->addPar("posY").setDoubleValue(currentPos.y);
    // Send the message
    sendDown(msg);

    // Log using the content variable (not msgContent)
    EV_INFO << simTime() << " Node " << getParentModule()->getId()
            << " broadcasted a BaseFrame1609_4 message with data: \""
            << content << "\"" << endl;
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

    EV_INFO << "onWSM called for message: " << (wsm ? wsm->getName() : "nullptr") << endl;

        if (!wsm) {
            EV_ERROR << "Received null message in onWSM!" << endl;
            // Handle or return if wsm is unexpectedly null
            return;
        }

        EV_INFO << "Received WSM details: Name=" << wsm->getName()
                    << ", Kind=" << wsm->getKind()
                    << ", ID=" << wsm->getId()
                    << ", ByteLength=" << wsm->getByteLength() << endl;

    // Note: To get the actual data content (pos, speed, etc.) from a received message,
    // it must have been sent using a custom message type with those fields, and you
    // would dynamic_cast the received wsm to that custom type and access its fields.

        // This is a standard way to get the sender's module ID in OMNeT++
           int senderNodeId = -1; // Default to invalid ID
           cGate* arrivalGate = wsm->getArrivalGate(); // Get the gate the message arrived through

           // Check if the arrival gate is valid and has an opposite gate connected
           if (arrivalGate && arrivalGate->getPreviousGate()) { // *** Use getOppositeGate() here ***
               // Get the module connected to the getRemoteGate end of the connection (the sender's module)
               cModule* senderModule = arrivalGate->getPreviousGate()->getOwnerModule(); // *** Use getOppositeGate() here ***
               if (senderModule) {
                   // Get the ID of the sender module
                   senderNodeId = senderModule->getId();
                   EV_INFO << "Sender Module ID obtained from arrival gate: " << senderNodeId << endl;
               } else {
                    EV_ERROR << "Sender module is null from opposite gate. Cannot determine sender ID." << endl;
               }
           } else {
               EV_ERROR << "Arrival gate or opposite gate is null. Cannot determine sender ID." << endl;
           }
    if (logFile.is_open()) {



        // Get information needed for the log entry
        double receiveTime = simTime().dbl(); // Convert simTime to double using dbl()
        int receiverNodeId = getParentModule()->getId(); // ID of the node that received the message

        // Get sender node ID from the message
        // In newer Veins versions, the structure has changed
        int senderNodeId = -1; // Default value in case we can't determine sender

        // Try to get sender info from the control info if available
        if (wsm->getControlInfo()) {
            cObject* ctrlInfo = wsm->getControlInfo();
            // You may need to check what type of control info is attached
            // and extract sender info accordingly
            // This might vary based on your Veins version
        }

        // Alternative: If you're using DemoBaseApplLayer's method to send WSMs,
        // try to cast to a DemoServiceAdvertisment
        veins::DemoServiceAdvertisment* dsa = dynamic_cast<veins::DemoServiceAdvertisment*>(wsm);
        if (dsa) {
            // If cast successful, use the fields available in DemoServiceAdvertisment
            senderNodeId = dsa->getSenderModuleId();
        }

        // What "Message Content" to log? For now, the message name.
        std::string messageInfo = wsm->getName(); // Get the message name (e.g., "v2v_broadcast")

        // Write the log entry in the desired format: [time] Sender -> Receiver : "Content"
        logFile << "[" << receiveTime << "s]\t" // Timestamp
                << "Node_" << senderNodeId // Sender ID (formatted)
                << "\tNode_" << receiverNodeId // Receiver ID (formatted)
                << "\t:\t\"" // Separator and opening quote
                << messageInfo << "\"\n"; // Message info/content and newline

        logFile.flush(); // Ensure data is written to file immediately (optional but useful for debugging)
    }

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
    // --- File Closing Code ---
        // Close the log file if it was successfully opened
        if (logFile.is_open()) {
            logFile << "------ Simulation Run Finished at " << simTime() << " ------\n";
            logFile.close();
        }
        // --- End File Closing Code ---

        // Clean up dynamically created objects (like timers)
        cancelAndDelete(broadcastTimer);

}
