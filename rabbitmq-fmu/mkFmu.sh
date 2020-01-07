

bin=$(readlink -f $1)
md=$(readlink -f $2)

mkdir fmu
cd fmu

mkdir -p binaries/darwin64/
mkdir resources

cp $md .
cp $md resources/
cp $bin binaries/darwin64/rabbitmq.dylib

zip -r ../rabbitmq.fmu .

cd ..
rm -rf fmu

echo $(readlink -f rabbitmq.fmu)
