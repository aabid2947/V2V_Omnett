// In file: MyBangaloreApp/MyBangaloreApp.cc

// Include standard OMNeT++ header (provides many core classes)
#include <omnetpp.h>

// Include your custom application header FIRST
#include "MyBangaloreApp.h"

// Include specific Veins headers needed
#include "veins/modules/mobility/traci/TraCIMobility.h" // For getting position from TraCI
#include "veins/base/utils/Coord.h" // For position data type

// Include headers for message types you handle or send
#include "veins/modules/messages/BaseFrame1609_4_m.h" // For BaseFrame1609_4 definition
#include "veins/modules/messages/DemoSafetyMessage_m.h" // For DemoSafetyMessage if handled
#include "veins/modules/messages/DemoServiceAdvertisement_m.h" // For DemoServiceAdvertisement if handled

// Include headers for channel/access constants and MAC/PHY modules (based on your reference)
// If veins::Channel::cch and veins::Access::immediate cause errors, you might need to try
// including Mac1609_4.h or PhyLayer80211p.h and finding where these constants are defined.
#include "veins/modules/mac/ieee80211p/Mac1609_4.h" // Might contain necessary constants or include them
// #include "veins/modules/phy/PhyLayer80211p.h" // Alternative include if constants not in Mac1609_4.h

// Include standard C++ headers
#include <fstream>   // For file stream operations (ofstream)
#include <sstream>   // For stringstream (building strings efficiently)
#include <string>    // For std::string and string manipulation
#include <iostream>  // For standard input/output (used by EV_ERROR, EV_INFO)
#include <cmath>     // For mathematical functions like std::round, std::fabs

// Include for temporary Sleep() function on Windows (REMOVE AFTER DEBUGGING)
// Uncomment the line below if you need to use Sleep() for debugging pauses.
// #include <windows.h>

// Use namespaces for cleaner code
using namespace omnetpp;      // Use OMNeT++ namespace
using namespace veins;       // Use Veins namespace
using namespace bangalore_v2v; // Use your project's namespace

// Register your custom application module
Define_Module(bangalore_v2v::MyBangaloreApp);


// Constructor (optional)
MyBangaloreApp::MyBangaloreApp() {
    // Initialize member pointers to nullptr in the constructor
    broadcastTimer = nullptr;
    // logFile is an object, doesn't need explicit nullptr initialization
}

// Destructor
MyBangaloreApp::~MyBangaloreApp() {
    // Clean up dynamically created objects (like scheduled messages)
    cancelAndDelete(broadcastTimer);
    // The logFile stream will be closed automatically when the object is destroyed,
    // but explicitly closing in finish() is good practice.
}

// Initialize method: Called when the module is created and initialized
void MyBangaloreApp::initialize(int stage) {
    // Always call the base class initialize first
    // DemoBaseApplLayer handles mobility initialization and other base features
    DemoBaseApplLayer::initialize(stage);

    // Stage 0 is typically used for basic setup, initializing variables, opening files
    if (stage == 0) {
        // Get the broadcast interval parameter from omnetpp.ini
        // Ensure "broadcastInterval" parameter is defined in your omnetpp.ini
        broadcastInterval = par("broadcastInterval").doubleValueInUnit("s");

        // Get the periodic logging interval parameter
        periodicLoggingInterval = par("periodicLoggingInterval").doubleValueInUnit("s");

        // Get the safety distance threshold parameter
        safetyDistanceThreshold = par("safetyDistanceThreshold").doubleValueInUnit("m");


        // Create a cMessage object for the periodic broadcast timer
        broadcastTimer = new cMessage("v2v_broadcast_timer");

        // Create a cMessage object for the periodic logging timer
        periodicTimer = new cMessage("v2v_periodic_log_timer");

        // Open the single custom log file for writing (append mode)
        // File name includes the module ID to create a separate log file for each node
        std::string logFileName = "v2v_log_node_" + std::to_string(getParentModule()->getId()) + ".txt";
        logFile.open(logFileName, std::ios_base::app); // Open in append mode

        if (!logFile.is_open()) {
            EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Could not open log file '" << logFileName << "'." << endl;
            // Consider throwing an error here if logging is essential for your simulation
            // throw cRuntimeError("Failed to open log file for node %d", getParentModule()->getId());
        } else {
            EV_INFO << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Simulation started. Log file '" << logFileName << "' opened for appending." << endl;
            // Write header row for the combined log file columns
            // Note: Different LogTypes will have different column usage, but the header defines all possible columns.
            // Columns: Time [s]    LogType Vehicle ID  PosX    PosY    Speed   NumNeighbors    AvgDistanceToNeighbors  SafetyLabel Sender ID   Receiver ID RxPosX  RxPosY  RxSpeed RxAccel Message Content Delay [s]   Content Byte Length Packet Byte Length
            logFile << "------ Simulation Run Started at " << simTime().dbl() << " ------\n";
            logFile << "Time [s]\tLogType\tVehicle ID\tPosX\tPosY\tSpeed\tNumNeighbors\tAvgDistanceToNeighbors\tSafetyLabel\tSender ID\tReceiver ID\tRxPosX\tRxPosY\tRxSpeed\tRxAccel\tMessage Content\tDelay [s]\tContent Byte Length\tPacket Byte Length\n"; // Combined Header with new Packet Byte Length column
            // Add notes about column usage based on LogType
            logFile << "# LogType: PERIODIC - Columns: Time, LogType, Vehicle ID, PosX, PosY, Speed, NumNeighbors, AvgDistanceToNeighbors, SafetyLabel\n";
            logFile << "# LogType: MESSAGE - Columns: Time, LogType, Receiver ID, N/A, N/A, N/A, N/A, N/A, N/A, Sender ID, Receiver ID, RxPosX, RxPosY, RxSpeed, RxAccel, Message Content, Delay [s], Content Byte Length\n";
            logFile << "# LogType: INPUT_READY - Columns: Time, LogType, Vehicle ID\n";
            logFile << "# LogType: ACTIVATION_SEND - Columns: Time, LogType, Sender ID, Receiver ID (Server), N/A, N/A, N/A, N/A, N/A, N/A, N/A, N/A, N/A, N/A, N/A, N/A, N/A, Packet Byte Length\n";
            logFile << "# LogType: GRADIENT_RECEIVE - Columns: Time, LogType, Receiver ID, N/A, N/A, N/A, N/A, N/A, N/A, Sender ID (Server), Receiver ID, N/A, N/A, N/A, N/A, N/A, N/A, Packet Byte Length\n";

            logFile.flush(); // Ensure the header is written immediately to the file
        }


        // Initialize counters for total statistics
        messagesSent = 0;
        totalBytesTransmitted = 0;
        messagesReceived = 0;
        totalBytesReceived = 0;

        // Initialize neighbor position map
        lastKnownNeighborPosition.clear();

    } else if (stage == 1) {
        // Stage 1 is often used when mobility is initialized and positions are available.
        // Schedule the first broadcast after mobility is ready.
        // Add a small random delay to the first broadcast to avoid synchronization issues.
        scheduleAt(simTime() + uniform(0.0, broadcastInterval), broadcastTimer);

        // Schedule the first periodic logging event
        // Add a small random delay to the first periodic log event as well
        scheduleAt(simTime() + uniform(0.0, periodicLoggingInterval), periodicTimer);
    }
}

// handleMessage method: Called for messages arriving at this module (from lower layers or self-messages)
void MyBangaloreApp::handleMessage(omnetpp::cMessage* msg) {
    // Check if the message is a gradient message from the server
    // This assumes gradient messages have a parameter "isGradient" set to true
    // You might need to adjust this check based on how your server module sends gradients
    if (msg->hasPar("isGradient") && msg->par("isGradient").boolValue()) {
        handleGradientFromServer(dynamic_cast<veins::BaseFrame1609_4*>(msg));
        return; // Message handled, return
    }

    // If not a gradient message, call the base class handleMessage first.
    // DemoBaseApplLayer's handleMessage will route other messages from lower layers
    // (arriving via the 'lowerGate') to methods like handleLowerMsg, onWSM, onBSM, etc.
    // It also handles self-messages by routing them to handleSelfMsg.
    DemoBaseApplLayer::handleMessage(msg);
}

// handleSelfMsg method: Called by DemoBaseApplLayer::handleMessage for self-messages
void MyBangaloreApp::handleSelfMsg(omnetpp::cMessage* msg) {
    // Check if the received message is the broadcast timer
    if (msg == broadcastTimer) {
        // It's time to send a periodic broadcast message (or activations)
        // In a true Split Learning setup, this timer might trigger sending activations
        // For now, we keep sendBroadcast for compatibility with previous code
        sendBroadcast(); // This might be replaced by sendActivationsToServer in a full implementation
        // Reschedule the timer for the next broadcast
        scheduleAt(simTime() + broadcastInterval, broadcastTimer);
    }
    // Check if the received message is the periodic logging timer
    else if (msg == periodicTimer) {
        // It's time to log periodic state and neighbor data, and the INPUT_READY timestamp

        // Get current simulation time
        simtime_t currentTime = simTime();
        int vehicleId = getParentModule()->getId();

        // --- Log INPUT_READY timestamp ---
        if (logFile.is_open()) {
            logFile << currentTime.dbl() << "\tINPUT_READY\t"
                    << vehicleId << "\n"; // Only log time, LogType, Vehicle ID
            logFile.flush();
        } else {
             EV_ERROR << currentTime.dbl() << ": Node " << vehicleId << ": Log file is NOT open! Cannot log INPUT_READY." << endl;
        }
        // --- End Log INPUT_READY timestamp ---


        // Get current vehicle state (position and speed)
        veins::TraCIMobility* mobilityModule = dynamic_cast<veins::TraCIMobility*>(getParentModule()->getSubmodule("veinsmobility"));
        veins::Coord currentPos(0, 0, 0);
        double currentSpeed = 0.0;
        // Acceleration is not directly available periodically from getSpeed().
        // It needs to be calculated in post-processing from logged speed/time.
        // We will log a placeholder here.
        double currentAcceleration = 0.0; // Placeholder for periodic log

        if (mobilityModule) {
            currentPos = mobilityModule->getPositionAt(currentTime);
            currentSpeed = mobilityModule->getSpeed();
        } else {
             EV_ERROR << currentTime.dbl() << ": Node " << vehicleId << ": Mobility module not found in periodic timer handler." << endl;
             // Cannot log state or neighbor info without mobility, reschedule and return.
             scheduleAt(simTime() + periodicLoggingInterval, periodicTimer); // Reschedule even on error
             return;
        }

        // --- Calculate Neighbor Information and Safety Label ---
        int numNeighbors = 0;
        double totalDistanceToNeighbors = 0.0;
        double avgDistanceToNeighbors = 0.0;
        double minDistanceToNeighbor = std::numeric_limits<double>::max(); // Initialize with a large value

        // Iterate through the map of last known neighbor positions
        for (auto const& [neighborId, neighborPos] : lastKnownNeighborPosition) {
            // You might want to add a check here to see if the last update time for this neighbor
            // is recent enough to consider them an active neighbor. For simplicity now,
            // we consider anyone in the map a neighbor.

            // Calculate distance between current vehicle position and neighbor's last known position
            double distance = std::sqrt(std::pow(currentPos.x - neighborPos.x, 2) + std::pow(currentPos.y - neighborPos.y, 2));

            numNeighbors++;
            totalDistanceToNeighbors += distance;

            // Update minimum distance
            if (distance < minDistanceToNeighbor) {
                minDistanceToNeighbor = distance;
            }
        }

        // Calculate average distance if there are neighbors
        if (numNeighbors > 0) {
            avgDistanceToNeighbors = totalDistanceToNeighbors / numNeighbors;
        }
        // If numNeighbors is 0, avgDistanceToNeighbors remains 0.0, minDistanceToNeighbor remains max double

        // --- Determine Safety Label based on minimum distance ---
        int safetyLabel = 0; // Default to Safe (0)
        // If there are neighbors AND the minimum distance is below the threshold, mark as Risky (1)
        if (numNeighbors > 0 && minDistanceToNeighbor < safetyDistanceThreshold) {
            safetyLabel = 1; // Risky (1)
        }
        // Note: You can add more complex logic here based on other factors (e.g., relative speed, TTC)

        // --- Log Periodic Data to the single log file ---
        if (logFile.is_open()) { // Use the single logFile
            std::stringstream logEntryStream;
            logEntryStream << currentTime.dbl() << "\t" // Current Simulation Time
                                   << "PERIODIC\t" // Log Type Indicator
                                   << vehicleId << "\t" // Vehicle ID
                                   << currentPos.x << "\t" // Current Position X
                                   << currentPos.y << "\t" // Current Position Y
                                   << currentSpeed << "\t" // Current Speed
                                   << numNeighbors << "\t" // Number of Neighbors
                                   << avgDistanceToNeighbors << "\t" // Average Distance to Neighbors
                                   << safetyLabel << "\n"; // Safety Label (end of PERIODIC specific columns)
                                   // Remaining columns for MESSAGE/Communication logs will be empty for this line


            logFile << logEntryStream.str();
            logFile.flush(); // Ensure data is written immediately

        } else {
             EV_ERROR << currentTime.dbl() << ": Node " << vehicleId << ": Log file is NOT open in periodic timer handler! Cannot log periodic data." << endl;
        }
        // --- End Log Periodic Data ---

        // Reschedule the timer for the next periodic log
        scheduleAt(simTime() + periodicLoggingInterval, periodicTimer);
    }
    else {
        // If it's not one of our known self-messages, call the base class self-message handler
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

// sendBroadcast method: Creates and sends a broadcast message
// In a Split Learning context, this might be adapted to send activations
void MyBangaloreApp::sendBroadcast() {
    veins::TraCIMobility* mobilityModule = dynamic_cast<veins::TraCIMobility*>(getParentModule()->getSubmodule("veinsmobility"));
    if (!mobilityModule) {
        EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Mobility module not found! Cannot send broadcast." << endl;
        return;
    }

    veins::BaseFrame1609_4* msg = new veins::BaseFrame1609_4("v2v_broadcast");

    std::stringstream contentStream;
    contentStream << "Node: " << getParentModule()->getId()
                  << ", Pos: " << mobilityModule->getPositionAt(simTime()).info()
                  << ", SendTime:" << simTime().dbl();

    std::string content = contentStream.str();
    msg->addPar("content").setStringValue(content.c_str());

    // --- Simulate adding activation data (placeholder) ---
    // In a real implementation, you would serialize activations here
    // For logging byte length, we can add a dummy parameter or calculate based on expected size
    // Let's add a dummy parameter whose size can be controlled for bandwidth evaluation simulation
    int activation_size_bytes = 512; // Example size, adjust as needed for simulation
    msg->addPar("activations_dummy").setStringValue(std::string(activation_size_bytes, 'X').c_str()); // Add a dummy string of specified size
    // --- End Simulate adding activation data ---


    msg->setChannelNumber(static_cast<int>(veins::Channel::cch));
    // msg->setChannelAccess(veins::Access::immediate); // Uncomment if needed and available

    // --- Log ACTIVATION_SEND event ---
    simtime_t sendTime = simTime();
    int packetByteLength = msg->getByteLength(); // Get total message size including headers and dummy data

    if (logFile.is_open()) {
        // LogType, Sender ID, Receiver ID (Server - assumed ID 0 or similar), Packet Byte Length
        logFile << sendTime.dbl() << "\tACTIVATION_SEND\t"
                << getParentModule()->getId() << "\t" << 0 << "\t" // Assuming Server ID is 0
                << "\t\t\t\t\t\t\t\t\t\t\t\t" // Empty columns for other log types
                << packetByteLength << "\n";
        logFile.flush();
    } else {
         EV_ERROR << sendTime.dbl() << ": Node " << getParentModule()->getId() << ": Log file is NOT open! Cannot log ACTIVATION_SEND." << endl;
    }
    // --- End Log ACTIVATION_SEND event ---


    sendDown(msg);

    messagesSent++;
    totalBytesTransmitted += packetByteLength; // Count the full packet size

    EV_INFO << simTime().dbl() << ": Node " << getParentModule()->getId() << " broadcasted a BaseFrame1609_4 message (" << packetByteLength << " bytes) at t=" << simTime().dbl() << " with content parameter: \"" << content << "\"" << endl;
}

// onWSM method: Called by DemoBaseApplLayer::handleLowerMsg for received BaseFrame1609_4 messages
// This method will now primarily handle non-gradient messages (like broadcasts from other vehicles)
void MyBangaloreApp::onWSM(veins::BaseFrame1609_4* wsm) {
    // Check if the message is a gradient message (should have been handled in handleMessage)
    if (wsm->hasPar("isGradient") && wsm->par("isGradient").boolValue()) {
        // This should not happen if handleMessage is routing correctly, but as a safeguard:
        EV_WARN << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Received gradient message in onWSM! Should be handled in handleMessage." << endl;
        handleGradientFromServer(wsm); // Pass to the correct handler
        return;
    }

    // Handle other WSM messages (like broadcasts from other vehicles)
    if (!wsm) {
        EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Received null message pointer in onWSM! Aborting processing." << endl;
        delete wsm; // Delete the message
        return;
    }

    std::string receivedContent;
    if (wsm->hasPar("content")) {
        const char* contentCStr = wsm->par("content").stringValue();
        if (contentCStr) {
            receivedContent = std::string(contentCStr);
        } else {
             EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Received WSM message with null 'content' parameter string value!" << endl;
             delete wsm;
             return;
        }
    } else {
        EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Received WSM message without 'content' parameter! Cannot extract content." << endl;
        delete wsm;
        return;
    }

    simtime_t sendTime = -1.0;
    // Find the "SendTime:" substring
    size_t sendTimePos = receivedContent.find("SendTime:");
    if (sendTimePos != std::string::npos) {
        std::string timeStr = receivedContent.substr(sendTimePos + 9);
        try {
            sendTime = std::stod(timeStr);
        } catch (const std::exception& e) {
            EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Error converting send time string '" << timeStr << "' to double from content: \"" << receivedContent << "\" : " << e.what() << endl;
        }
    } else {
        EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Could not find 'SendTime:' in received message content: \"" << receivedContent << "\"" << endl;
    }

    int senderNodeId = -1;
    cGate* arrivalGate = wsm->getArrivalGate();
    // Corrected: Use getPreviousGate() to get the gate on the sender's side
    if (arrivalGate && arrivalGate->getPreviousGate()) {
         cModule* senderModule = arrivalGate->getPreviousGate()->getOwnerModule();
         if (senderModule) {
             senderNodeId = senderModule->getId();
         } else {
              EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Sender module is null from previous gate. Cannot determine sender ID." << endl;
         }
    } else {
         EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Arrival gate or previous gate is null. Cannot determine sender ID." << endl;
    }


    simtime_t receiveTime = simTime();
    double delay = -1.0;
    if (sendTime >= 0) {
        delay = (receiveTime - sendTime).dbl();
        if (delay < 0) delay = 0;
    }

    veins::TraCIMobility* mobilityModule = dynamic_cast<veins::TraCIMobility*>(getParentModule()->getSubmodule("veinsmobility"));
    double receiverPosX = 0.0, receiverPosY = 0.0;
    double receiverSpeed = 0.0;
    double receiverAcceleration = 0.0; // Placeholder

    if (mobilityModule) {
        veins::Coord receiverPos = mobilityModule->getPositionAt(simTime());
        receiverPosX = receiverPos.x;
        receiverPosY = receiverPos.y;
        receiverSpeed = mobilityModule->getSpeed();
        // Acceleration is not directly available from getSpeed().
        // The RxAccel field in the log is likely from SUMO, not calculated by Veins.
        // We will log the RxAccel value from the log line if it's available in the parsed content later.
    } else {
         EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Mobility module not found in onWSM for receiver state." << endl;
    }

    int receiverNodeId = getParentModule()->getId();
    int contentByteLength = receivedContent.length();
    int packetByteLength = wsm->getByteLength(); // Total packet byte length


    // --- Update last known position of the sender (neighbor) ---
    // Extract sender position from the content string
    veins::Coord senderPos(0, 0, 0); // Default position

    // Declare posStrPos, posStart, and posEnd here so they are accessible later
    size_t posStrPos = std::string::npos;
    size_t posStart = std::string::npos;
    size_t posEnd = std::string::npos;

    posStrPos = receivedContent.find("Pos: (");
    if (posStrPos != std::string::npos) {
        posStart = posStrPos + 6; // Length of "Pos: ("
        posEnd = receivedContent.find(")", posStart);
        if (posEnd != std::string::npos) {
            std::string posCoordsStr = receivedContent.substr(posStart, posEnd - posStart);
            std::stringstream ss(posCoordsStr);
            std::string segment;
            std::vector<double> coords;
            while(std::getline(ss, segment, ',')) {
                try {
                    coords.push_back(std::stod(segment));
                } catch (const std::exception& e) {
                    EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Error converting sender position coordinate '" << segment << "' from content: \"" << receivedContent << "\" : " << e.what() << endl;
                    // Clear coords if parsing fails for safety
                    coords.clear();
                    break;
                }
            }
            if (coords.size() >= 2) { // Need at least X and Y
                senderPos.x = coords[0];
                senderPos.y = coords[1];
                // Z is coords[2] if needed
            } else {
                 EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Could not parse sender position coordinates from content: \"" << receivedContent << "\"" << endl;
            }
        } else {
             EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Could not find closing parenthesis for sender position in content: \"" << receivedContent << "\"" << endl;
        }
    } else {
         EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Could not find 'Pos: (' in received message content: \"" << receivedContent << "\"" << endl;
    }

    // If we successfully extracted the sender ID (from gate) and sender position (from content),
    // update the last known position map.
    if (senderNodeId != -1 && posStrPos != std::string::npos && posEnd != std::string::npos) {
         lastKnownNeighborPosition[senderNodeId] = senderPos;
         // EV_INFO << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Updated last known position for neighbor " << senderNodeId << " to " << senderPos.info() << endl;
    }
    // --- End Update last known position ---


    // --- Log Received Message Details to the single log file ---
    if (logFile.is_open()) { // Use the single logFile
        std::stringstream logEntryStream;
        logEntryStream << receiveTime.dbl() << "\t" // Timestamp of reception
                               << "MESSAGE\t" // Log Type Indicator
                               << receiverNodeId << "\t" // Vehicle ID (Receiver)
                               << receiverPosX << "\t" // Receiver Position X
                               << receiverPosY << "\t" // Receiver Position Y
                               << receiverSpeed << "\t" // Receiver Speed
                               << "N/A\tN/A\t" // Placeholder for NumNeighbors and AvgDistance (only available periodically)
                               << "N/A\t" // Placeholder for Safety Label
                               << (senderNodeId != -1 ? std::to_string(senderNodeId) : "Unknown") << "\t" // Sender ID
                               << receiverNodeId << "\t" // Receiver ID
                               << receiverPosX << "\t" // RxPosX (Receiver's X at time of Rx)
                               << receiverPosY << "\t" // RxPosY (Receiver's Y at time of Rx)
                               << receiverSpeed << "\t" // RxSpeed (Receiver's Speed at time of Rx)
                               << receiverAcceleration << "\t" // RxAccel (Placeholder - acceleration at time of Rx)
                               << "\"" << receivedContent << "\"\t" // Message Content (Quoted to handle spaces/tabs)
                               << delay << "s\t" // Calculated Delay
                               << contentByteLength << "\t" // Content Byte Length
                               << packetByteLength << "\n"; // Total Packet Byte Length

        logFile << logEntryStream.str();
        logFile.flush();

    } else {
         EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Log file is NOT open in onWSM! Cannot log received message details." << endl;
    }
    // --- End Log Received Message Details ---


    messagesReceived++;
    totalBytesReceived += packetByteLength; // Count the full packet size

    delete wsm;
}


// handleGradientFromServer method: Handles messages identified as gradients from the server
void MyBangaloreApp::handleGradientFromServer(veins::BaseFrame1609_4* msg) {
    if (!msg) {
        EV_ERROR << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Received null gradient message pointer! Aborting processing." << endl;
        return;
    }

    simtime_t receiveTime = simTime();
    int receiverNodeId = getParentModule()->getId();
    int packetByteLength = msg->getByteLength(); // Total packet byte length

    // --- Log GRADIENT_RECEIVE event ---
    if (logFile.is_open()) {
        // LogType, Receiver ID, Sender ID (Server - assumed ID 0 or similar), Packet Byte Length
        logFile << receiveTime.dbl() << "\tGRADIENT_RECEIVE\t"
                << receiverNodeId << "\t" << 0 << "\t" // Assuming Server ID is 0
                << "\t\t\t\t\t\t\t\t\t\t\t\t\t" // Empty columns for other log types
                << packetByteLength << "\n";
        logFile.flush();
    } else {
         EV_ERROR << receiveTime.dbl() << ": Node " << receiverNodeId << ": Log file is NOT open! Cannot log GRADIENT_RECEIVE." << endl;
    }
    // --- End Log GRADIENT_RECEIVE event ---

    // --- Process Gradients (Simulated) ---
    // In a real Split Learning implementation, you would deserialize the gradients
    // and perform the client-side backward pass here.
    // For this simulation, we just log the event.
    EV_INFO << simTime().dbl() << ": Node " << receiverNodeId << ": Received gradient message (" << packetByteLength << " bytes) from server." << endl;
    // --- End Process Gradients ---

    delete msg; // Delete the received gradient message
}


// onBSM method: Called by DemoBaseApplLayer::handleLowerMsg for received DemoSafetyMessage (BSMs)
// Implement this if your simulation sends and receives BSMs explicitly.
// If you are only sending/receiving BaseFrame1609_4 as in your sendBroadcast(),
// the onWSM method above will be called.
/*
void MyBangaloreApp::onBSM(veins::DemoSafetyMessage* bsm) {
    // Example: Log that a BSM was received
    EV_INFO << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Received BSM from Node " << bsm->getSenderModuleId() << endl;

    // If you want to log BSMs to your file, add logging code here similar to onWSM.
    // Be mindful of the specific fields available in DemoSafetyMessage.

    // Delete the received message
    delete bsm;
}
*/

// onWSA method: Called by DemoBaseApplLayer::handleLowerMsg for received DemoServiceAdvertisement (WSAs)
// Implement this if your simulation sends and receives WSAs explicitly.
/*
void MyBangaloreApp::onWSA(veins::DemoServiceAdvertisement* wsa) {
    // Example: Log that a WSA was received
    EV_INFO << simTime().dbl() << ": Node " << getParentModule()->getId() << ": Received WSA from Node " << wsa->getSenderModuleId() << endl;

    // If you want to log WSAs to your file, add logging code here similar to onWSM.
    // Be mindful of the specific fields available in DemoServiceAdvertisement.

    // Delete the received message
    delete wsa;
}
*/

// handlePositionUpdate method: Called by DemoBaseApplLayer when the node's position is updated
// You can add custom logic here that needs to run whenever the vehicle moves.
void MyBangaloreApp::handlePositionUpdate(cObject* obj) {
    // Always call the base class method
    DemoBaseApplLayer::handlePositionUpdate(obj);

    // Add any custom logic needed after position update
    // Example: Check if position has changed significantly, react to map events, etc.
}


// finish method: Called at the end of the simulation run
void MyBangaloreApp::finish() {
    // Always call the base class finish method
    DemoBaseApplLayer::finish();

    // Log summary statistics for this node to the console.
    // These totals provide an overview of communication activity for this specific module instance.
    EV_INFO << "--- Simulation Statistics for Node " << getParentModule()->getId() << " ---" << endl;
    EV_INFO << "Total Messages Sent: " << messagesSent << endl;
    EV_INFO << "Total Bytes Transmitted: " << totalBytesTransmitted << " bytes" << endl;
    EV_INFO << "Total Messages Received: " << messagesReceived << endl;
    EV_INFO << "Total Bytes Received: " << totalBytesReceived << " bytes" << endl;
    EV_INFO << "--- End Statistics for Node " << getParentModule()->getId() << " ---" << endl;


    // Close the single log file
    if (logFile.is_open()) {
        logFile << "------ Simulation Run Finished at " << simTime().dbl() << " ------\n";
        logFile.close();
    }
}
