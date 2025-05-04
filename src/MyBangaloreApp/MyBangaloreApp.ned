// Place this file at: Bangalore_v2v/src/bangalore_v2v/MyBangaloreApp.ned

package bangalore_v2v.MyBangaloreApp;
// Import the base class
import org.car2x.veins.modules.application.ieee80211p.DemoBaseApplLayer;
// If extending covers interface checks, IBaseApplLayer import might not be strictly needed here,
// but it doesn't hurt to keep it for clarity if your code relies on it.
// import org.car2x.veins.base.modules.IBaseApplLayer;


simple MyBangaloreApp extends DemoBaseApplLayer
{
    parameters:
        @class(bangalore_v2v::MyBangaloreApp); // <-- Fixed namespace to match C++ file

        double broadcastInterval @unit(s) = default(1s);

        // headerLength is REMOVED - inherited from DemoBaseApplLayer
        // int headerLength @unit(bit) = default(88bit);

        bool logVehicleData = default(true);
        string logFileName = default("v2v_log.txt");

        @display("i=block/app2");


    // *** REMOVE THIS ENTIRE GATES SECTION: Gates are inherited from DemoBaseApplLayer ***
    // gates:
    //    input lowerLayerIn;
    //    output lowerLayerOut;
}