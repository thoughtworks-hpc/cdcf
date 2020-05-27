#!/usr/bin/env bash
echo "running on host: $HOST"
echo "seeds: $SEEDS"
echo /bin/node_keeper --name=$HOST --seeds=$SEEDS --host=$HOST --port=4445
/bin/node_keeper --name=$HOST --seeds=$SEEDS --host=$HOST --port=4445 &
echo $APP --host=$HOST
$APP --host=$HOST
