Windows only - built and tested in visual sutdio 2022

Small test to check whether a dll is being loaded.

Change paths accordingly. The path for ```ws2_32.dll``` should be standard, and loading this dll should work.

Loading the ```rabbitmq.dll``` should result in an error, i.e., no module handle (hmdoule) is created. 
Trying to load the libcrypto and libssl also fails with the same code ```126```. In the test these three libraries are in the same folder.

