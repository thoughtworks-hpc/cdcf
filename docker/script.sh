#!/usr/bin/env bash
echo "running on host: $HOST"
echo "seeds: $SEEDS"
echo /bin/node_keeper --name=$HOST --seeds=$SEEDS --host=$HOST --port=4445
/bin/node_keeper --name=$HOST --seeds=$SEEDS --host=$HOST --port=4445 &
echo $APP $BEROOT -H $APPHOST -p $APPPORT -n 4445 -w $WORKERPORT --threads_proportion=2
$APP $BEROOT -H $APPHOST -p $APPPORT -n 4445 -w $WORKERPORT --threads_proportion=2
