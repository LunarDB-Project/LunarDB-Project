database drop table_test;
database create table_test;
database use table_test;
schema table_schema { field1: String; field2: Integer; };
create table sometable based on table_schema;
insert into sometable objects [  
  { "field1": "val1", "field2": "2" }  
  { "field1": "val2", "field2": "123" }
];