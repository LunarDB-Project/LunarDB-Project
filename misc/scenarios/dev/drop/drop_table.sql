database drop test_db;
database create test_db;
database use test_db;
schema table_schema01 { field1: String; field2: Integer; };
schema table_schema02 { str: String; obj: table_schema01; };
create table sometable_join based on table_schema02;
insert into sometable_join objects [  
  { "str": "val1", "obj": { "field1": "val11", "field2": "1" } }  
  { "str": "val2", "obj": { "field1": "val22", "field2": "2" } }
];

select from structure sometable_join where ( 1 == 1 ) fields [ str, obj, obj.field1, obj.field2 ];
drop structure sometable_join;
select from structure sometable_join where ( 1 == 1 ) fields [ str, obj, obj.field1, obj.field2 ];
