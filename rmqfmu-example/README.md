# Running the example

In this example we will read data (x and y position) from the a csv file and send it to a co-simulation through the rabbitMQ server. 
The RMQFMU will thereafter propagate the data to another FMU that calculates the distance from point (0,0), and sends it back through the RMQFMU.

Preparing the rabbitMQ FMU (RMQFMU) through the following steps, or use the FMU provided in this folder:

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

Create the mabl specification for the co-simulation with two FMUs. Run in a new terminal:

```bash
$ java -jar maestro-2.3.0.jar import sg1 coe.json multimodel.json -output generate
```

Thereafter, run the co-simulation for 10 seconds (already defined in the ```coe.json``` file):

```bash
$ java -jar maestro-2.3.0.jar interpret generate/spec.mabl -output results
```

Inspect the generated ```outputs.csv``` file in the ```results``` folder.