// MyNewProject/src/bangalore_v2v/MyBangaloreApp.h
#ifndef BANGALORE_V2V_MYBANGALOREAPP_H_ // Updated include guard
#define BANGALORE_V2V_MYBANGALOREAPP_H_
#include "veins/veins.h" // Include fundamental Veins headers

// Include the direct base class header
#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"

// Include headers for message types you handle or send
#include "veins/modules/messages/DemoSafetyMessage_m.h"
// For onBSM handler

#include "veins/modules/messages/DemoServiceAdvertisement_m.h" // For onWSA handler
#include "veins/modules/messages/BaseFrame1609_4_m.h"          // For BaseFrame1609_4

#include "veins/modules/mobility/traci/TraCIMobility.h"     // Needed to get position/speed
#include "veins/base/utils/SimpleAddress.h"              // Needed for CCH

#include <fstream> // For file logging
#include <sstream> // For building message strings

using namespace omnetpp;
using namespace veins; // Keep using namespace for simpler code inside methods

// Change namespace to match your NED package name
namespace bangalore_v2v { // <-- Changed from myproject to bangalore_v2v


// Use veins:: prefix for base class
class MyBangaloreApp : public veins::DemoBaseApplLayer {
public:
    virtual void initialize(int stage) override;
    virtual void finish() override;
//private:
//    std::ofstream logFile;

protected:
    // Use veins:: prefix for message types in signatures
    virtual void onBSM(veins::DemoSafetyMessage* bsm) override;
    virtual void onWSM(veins::BaseFrame1609_4* wsm) override;
    virtual void onWSA(veins::DemoServiceAdvertisment* wsa) override;

    virtual void handleSelfMsg(cMessage* msg) override;
    virtual void handlePositionUpdate(cObject* obj) override;

    simtime_t broadcastInterval;
    cMessage* broadcastTimer;
    std::ofstream logFile;

    virtual void sendBroadcast();
};

} // namespace bangalore_v2v

#endif // BANGALORE_V2V_MYBANGALOREAPP_H_
