package bangalore_v2v.simulations.bangalore_simulation;

// Import the necessary modules from Veins
import org.car2x.veins.nodes.Car; // For the vehicle compound module
import org.car2x.veins.modules.mobility.traci.TraCIScenarioManagerLaunchd; // For the SUMO manager
import org.car2x.veins.modules.obstacle.ObstacleControl; // For obstacles (if used)
import org.car2x.veins.modules.world.annotations.AnnotationManager; // For annotations (if used)
import org.car2x.veins.base.connectionManager.ConnectionManager;
// Import your custom application
import org.car2x.veins.visualizer.roads.RoadsCanvasVisualizer;
import bangalore_v2v.MyBangaloreApp.MyBangaloreApp; // <-- Added this import
import org.car2x.veins.base.modules.BaseWorldUtility; // <-- ADD THIS LINE
network BangaloreScenario
{
    parameters:
        // Playground size parameters (will be overridden by TraCI based on SUMO map boundaries)
//        double playgroundSizeX @unit(m);
//        double playgroundSizeY @unit(m);
//        double playgroundSizeZ @unit(m);
//        @display("bgb=$playgroundSizeX,$playgroundSizeY");

        //	
        // Playground size parameters (will be overridden by TraCI based on SUMO map boundaries)
        double playgroundSizeX @unit(m);
        double playgroundSizeY @unit(m);
        double playgroundSizeZ @unit(m);
        // Comment out or remove the original line:
        // @display("bgb=$playgroundSizeX,$playgroundSizeY");
        // Add this line as a workaround:
         @display("bgb=dropUnit($playgroundSizeX),dropUnit($playgroundSizeY)");
        // <-- Use dimensionless numbers for background size

        // This parameter will be set by the TraCIScenarioManagerLaunchd
        // to the number of vehicles in the SUMO route file.
        int numCars;

    submodules:
        // The TraCI Scenario Manager module connects to SUMO and manages vehicles

        manager: TraCIScenarioManagerLaunchd {
            parameters:
                @display("p=697,451");
        }
        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeX", 1525, "parent.playgroundSizeX");
                playgroundSizeY = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeY", 1525, "parent.playgroundSizeY");
                playgroundSizeZ = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeZ", 1525, "parent.playgroundSizeZ");
                @display("p=30,0;i=misc/globe");
        }
        // Obstacle module (include only if you are using obstacles)
        //        obstacles: ObstacleControl {
        //            parameters:
        //                @display("p=42,90");
        //        }
        // Annotation manager (include only if you are using annotations)
        annotations: AnnotationManager {
            parameters:
                @display("p=66,679");
        }


roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=300,0");
        }

        connectionManager: ConnectionManager {
            parameters:
                @display("p=631,138");
                // You might need to set 'maxInterfDist' here if not set in omnetpp.ini
                // maxInterfDist = default(2500m); // Example default value
        }
        // Declare the submodule vector for the vehicle nodes.
        // The size of this vector is determined by the numCars parameter.
        node[numCars]: Car {
            parameters:
                // Use a different approach to specify applType
                appl.typename = "bangalore_v2v.MyBangaloreApp.MyBangaloreApp"; // <-- Simplified to just the module name
                // Pass the broadcastInterval parameter to your application module
                appl.broadcastInterval = default(1s); // Example default value, can be overridden in omnetpp.ini

                @display("i=vehicle/passenger;p=463,841");
        }

    connections allowunconnected:
        // Define connections between modules here if needed.
        // For V2V communication, connections are typically handled by the network interface cards (NICs)
        // and the connection manager, not explicitly listed here unless specific wired connections are involved.
        // 'allowunconnected' permits modules/gates to be unconnected.
}
