export LUNARDB_ROOT_PASSWORD="root"
rm -rf ./lunardb/databases/*
rm -rf ./lunardb/logs/*
rm -rf ./lunardb/schemas/*
rm -rf ./lunardb/wal/*
cp ./lunardb.config.json ./build/src/LunarDB/CLI/lunardb.config.json
./build/src/LunarDB/CLI/LunarDB_CLI.debug "$@"
