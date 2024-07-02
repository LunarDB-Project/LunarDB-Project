database drop table_test;
database create table_test;
database use table_test;
schema table_schema { field1: String; field2: Integer; };
create table sometable based on table_schema;
insert into sometable objects [  
  { "field1": "val1", "field2": "11" }  
  { "field1": "val2", "field2": "22" }
  { "field1": "val3", "field2": "33" }
  { "field1": "val4", "field2": "44" }
];
select from structure sometable where ( 1 == 1 ) fields [ field1, field2 ];
delete from structure sometable where ( field1 == val1 or field1 == val3 );
select from structure sometable where ( 1 == 1 ) fields [ field1, field2 ];