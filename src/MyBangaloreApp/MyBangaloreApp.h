// In file: MyBangaloreApp/MyBangaloreApp.h

#ifndef MYBANGALOREAPP_H_
#define MYBANGALOREAPP_H_

// Include the base Veins application layer class
#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"

// Include for file stream operations (for logging to a file)
#include <fstream>
// Include for stringstream (useful for building log strings)
#include <sstream>
// Include for string manipulation (std::string, std::to_string, std::stod)
#include <string>
// Include for standard input/output streams (like std::cerr, std::cout for EV messages)
#include <iostream>
// Include for mathematical functions (like std::round or std::fabs if needed)
#include <cmath>

namespace bangalore_v2v {

// MyBangaloreApp class inherits from DemoBaseApplLayer
class MyBangaloreApp : public veins::DemoBaseApplLayer {
protected:
    // Timer for periodic broadcasts
    omnetpp::cMessage* v2v_broadcast_timer = nullptr;

    // Custom log file stream for detailed message logging
    std::ofstream logFile;

    // Counters for logging total statistics at the end of the simulation
    unsigned long messagesSent;
    unsigned long totalBytesTransmitted;
    unsigned long messagesReceived;
    unsigned long totalBytesReceived;
    // --- New members for Periodic Logging ---
    // --- Members for Periodic Logging (now logging to the single logFile) ---
       omnetpp::cMessage* periodicTimer; // Timer for periodic state and neighbor logging
//       simtime_t periodicLoggingInterval; // Interval for periodic logging (e.g., 0.1s)

       // Data structure to store last known position of neighbors (based on received messages)
       std::map<int, veins::Coord> lastKnownNeighborPosition;
       // You might want a timer for cleaning up old entries in this map if neighbors move out of range

       // --- New member for Safety Labeling ---
       double safetyDistanceThreshold; // Distance threshold to consider a situation "risky"




public:
    // Constructor (optional, but good practice)
    MyBangaloreApp();
    // Destructor (important for cleaning up resources)
    virtual ~MyBangaloreApp();

protected:
    // OMNeT++ methods to override
    // initialize(): Called when the module is created and initialized
    virtual void initialize(int stage) override;
    // handleMessage(): Called when a message arrives at the module (from lower layers or self-messages)
    virtual void handleMessage(omnetpp::cMessage* msg) override;
    // finish(): Called at the end of the simulation run
    virtual void finish() override;
    virtual void handleSelfMsg(omnetpp::cMessage* msg) override;
        virtual void handlePositionUpdate(cObject* obj) override;
        // --- New/Modified methods for Evaluation Metrics Logging ---
        void handleGradientFromServer(veins::BaseFrame1609_4* msg); // Method to handle received gradients


    // Veins method to override for received WSM messages
    // onWSM(): Called by the base class when a BaseFrame1609_4 message is received from lower layers
    virtual void onWSM(veins::BaseFrame1609_4* wsm) override;

    // Custom method to create and send broadcast messages
    virtual void sendBroadcast();
    simtime_t periodicLoggingInterval;
    simtime_t broadcastInterval;
      cMessage* broadcastTimer;
    // You can add other custom methods needed for your application logic here
};

} // namespace bangalore_v2v

#endif /* MYBANGALOREAPP_H_ */
