#!/usr/bin/env bash
docker-compose -f yanghui_app.yml --env-file ssl.env up -d