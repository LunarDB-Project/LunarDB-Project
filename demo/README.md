# Agendă

## 1. LunarDB Server (C++, JS)

### 1.1. Aplicația Web ”AstroHuddle”

```
./scripts/release/server.sh
tail -f lunardb/logs/lunardb.log

node start
```

### 1.2. Conexiune C++ <-> JavaScript

Teste de integrare, Logging, Scenariu

```
# test de stres
node ./test/integration/scenario.03.integration-test.js

# scenariu business
node ./test/integration/scenario.04.integration-test.js
```

## 2. LunarDB CLI, Scenarii practice

### 1.1. ”Business”

```
./scripts/release/cli.sh ./demo/business.sql
```

### 1.2. ”CRUD” Generic

```
./scripts/release/cli.sh ./demo/crud.sql
./scripts/release/cli.sh ./demo/select.sql
./scripts/release/cli.sh ./demo/rollback_table.sql
./scripts/release/cli.sh ./demo/rollback_document.sql
```

### 1.2. ”WriteAheadLog ~ Crash Recovery”

```
./scripts/release/cli.sh ./demo/large/table_insert.sql
./scripts/release/cli.sh ./demo/large/collection_insert.sql
./scripts/release/cli.sh ./demo/large/collection_update.sql

cat lunardb/wal/wal.log
./scripts/release/cli.sh
tail -f lunardb/logs/lunardb.log

cat lunardb/databases/WAL_Scenario_DB_table/collections/WAL_Scenario_Collection_table/data/data.ldbtbl | cut -c-5000
tree lunardb/databases
find lunardb/databases -name '*ldbdoc' | xargs cat | grep 1000
```
