# 🌙 Moonlight Query Language

Moonlight Query Language _(MQL)_ is a versatile and powerful query language developed for use with LunarDB. It provides a consistent and unified syntax for managing and querying data, regardless of whether the data is structured or unstructured.

MQL simplifies database interactions by offering a single language for all data types, making it easier to perform complex queries, updates, and data manipulations. This approach ensures that users can efficiently handle diverse datasets without the need to switch between different query languages or paradigms.

With MQL, you can leverage the full capabilities of LunarDB to store, retrieve, and manage your data seamlessly, ensuring high performance and scalability for your applications. Whether you're working with relational data, JSON documents, or any other data format, MQL provides the tools you need to get the job done effectively.

### 1. 🗄️ Database & Users management

```
database create <database-name>;
database use <database-name>;
database drop <database-name>;
database backup <database-name> to disk "path/to/backup-directory";

user create <username> password <password>;

auth <username>;

grant [ command-01, command-02, ..., command-n] to <username> on <collection-name>;
revoke [ command-01, command-02, ..., command-n] from <username> on <collection-name>;

```

### 2. 📁 Collections management

```
schema <schema-name> inherits <some-other-schema-01>, <some-other-schema-02>, ..., <some-other-schema-n> {
    field01: Type;
    field02: Type;
    ...
    field-n: Type;
};
Type: String|Boolean|DateTime|Integer|Float|Custom-Type like <some-other-schema-03>

create table|collection <collection-name> based on <schema-name>;
create table|collection <collection-name> based on <schema-name>
    binding [
        <field-name01> from <other-collection-name-01>,
        <field-name02> from <other-collection-name-02>
    ];

rebind <collection-name>::<field-name> to <other-collection-name>;
rebind <collection-name>::<field-name> to <other-collection-name> clean;

drop structure <collection-name>;
drop structure <collection-name> cascade;

truncate structure <collection-name>;
```

### 3. 📊 Data management

```
insert into <collection-name> objects [
    {
        // json object 01
    }
    {
        // json object 02
    }
    ...
    {
        // json object n
    }
];

select from structure <collection-name>
    where <where-clause>
    fields [ <field-name01>, <field-name02>, ..., <field-name-n> ];

delete from <collection-name>
    where <where-clause>;

update structure <collection-name>
    where <where-clause>
    modify [
        <field-name01> => expression01,
        <field-name02> => expression02,
        ...
        <field-name-n> => expression-n,
    ];

commit;
savepoint <hash>;
rollback <hash>;
```
