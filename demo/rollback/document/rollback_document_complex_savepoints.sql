database drop test_db;
database create test_db;
database use test_db;
schema documentschema01 { field1: String; field2: Integer; };
schema documentschema02 { str: String; obj: documentschema01; };

create collection somedocumenttest based on documentschema02;
select from structure somedocumenttest where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

savepoint 11;
insert into somedocumenttest objects [  
  { "str": "val11", "obj": { "field1": "val11", "field2": "11" } }  
  { "str": "val111", "obj": { "field1": "val111", "field2": "111" } }
  { "str": "val111", "obj": { "field1": "val111", "field2": "111" } }
  { "str": "val99", "obj": { "field1": "val99", "field2": "99" } }  
];
select from structure somedocumenttest where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

savepoint 22;
update structure somedocumenttest where ( str == val111 ) modify [ str => val222, obj.field1 => val222, obj.field2 => 222 ];
select from structure somedocumenttest where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

savepoint 33;
update structure somedocumenttest where ( obj.field1 == val222 ) modify [ str => val333, obj.field1 => val333, obj.field2 => 333 ];
select from structure somedocumenttest where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

rollback 33;
select from structure somedocumenttest where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

rollback 33;
select from structure somedocumenttest where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

rollback 22;
select from structure somedocumenttest where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

rollback 11;
select from structure somedocumenttest where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];
