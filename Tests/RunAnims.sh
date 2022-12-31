#!/bin/sh
set -e

for i in `seq 1 20`;
do
	build.x86_64/install/AnimProducer&
	sleep 0.3
done
