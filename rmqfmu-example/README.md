# Running the example

In this example we will read data (x and y position) from the given csv file and send it to a co-simulation through the rabbitMQ server. 
The Mabl language is used to generate a co-orchestration algorithm that runs the co-simulation, based on coe.json and multimodel.json files.
The co-simulation is run with the Maestro co-orchestration engine (the jar of which is included in this folder).
The RMQFMU will thereafter propagate the data to another FMU that calculates the distance from point (0,0), and sends it back through the RMQFMU.

Use the FMU provided in the rmqfmu-example folder, or prepare the RMQFMU through the following steps:

1. Download the RMQFMU from the release page. 
2. Unzip it, recall that an FMU is like anyother archive. 
3. Place the modelDescription.xml file located in the rmqfmu-example folder in the root of the unzipped FMU as well as in the resources folder. 
4. Compress the contents of the unzipped folder, and change the extension to ```.fmu```.

Setup the rabbitMQ server using docker. Go to the server folder and run:

```bash
$ docker-compose up -d
```

In a terminal run the scripts that will publish and consume from the RMQFMU:

```bash
$ python3 ./publish.py
```

In another terminal run:

```bash
$ python3 ./consume.py
```

Before running the co-simulation make sure the dependencies in ```requirements.txt``` are installed (e.g. using pip).
These are needed from the fmus which have been produced using UNIFMU.

Create the mabl specification for the co-simulation with two FMUs. Run in a new terminal:

```bash
$ java -jar maestro-2.3.0.jar import sg1 coe.json multimodel.json -output generate
```

Thereafter, run the co-simulation for 10 seconds (already defined in the ```coe.json``` file):

```bash
$ java -jar maestro-2.3.0.jar interpret generate/spec.mabl -output results
```

Inspect the generated ```outputs.csv``` file in the ```results``` folder.

## How to reuse the example files

For a co-simulation to run with the INTOCPS tool-chain, the coe.json and multimodel.json files will be needed, and the ones provided 
here can serve as a template, and can be modified directly to change and include other FMUs and their connections.
Similarly, the publish and consume scripts can be adapted to fit other needs, e.g., in terms of the connection configuration to the rabbitMQ server,
as well as by modifying the messages published to the RMQFMU, by adding other fields etc.
The provided maestro jar is only for purposes of this example, and the user is advised to download the latest release version from the Maestro repo.
The RMQFMU can also be tailored, and these configuration details are covered in the [Main Readme](README.md).

