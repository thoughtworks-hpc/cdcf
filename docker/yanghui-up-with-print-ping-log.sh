#!/usr/bin/env bash
docker-compose -f yanghui_app.yml --env-file print_ping_log.env up -d