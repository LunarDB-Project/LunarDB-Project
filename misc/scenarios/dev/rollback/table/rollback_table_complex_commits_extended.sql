database drop test_db;
database create test_db;
database use test_db;
schema table_schema01 { field1: String; field2: Integer; };
schema table_schema02 { str: String; obj: table_schema01; };

create table sometable_test based on table_schema02;
select from structure sometable_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

insert into sometable_test objects [  
  { "str": "val11", "obj": { "field1": "val11", "field2": "11" } }  
  { "str": "val111", "obj": { "field1": "val111", "field2": "111" } }
  { "str": "val111", "obj": { "field1": "val111", "field2": "111" } }
  { "str": "val99", "obj": { "field1": "val99", "field2": "99" } }  
];
select from structure sometable_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];
commit;

update structure sometable_test where ( str == val111 ) modify [ str => val222, obj.field1 => val222, obj.field2 => 222 ];
select from structure sometable_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

update structure sometable_test where ( obj.field1 == val222 ) modify [ str => val333, obj.field1 => val333, obj.field2 => 333 ];
select from structure sometable_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

rollback;
select from structure sometable_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];
