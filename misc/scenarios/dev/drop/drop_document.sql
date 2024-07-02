database drop test_db;
database create test_db;
database use test_db;
schema document_schema01 { field1: String; field2: Integer; };
schema document_schema02 { str: String; obj: document_schema01; };
create collection somedocument based on document_schema02;
insert into somedocument objects [  
  { "str": "val1", "obj": { "field1": "val11", "field2": "1" } }  
  { "str": "val2", "obj": { "field1": "val22", "field2": "2" } }
];
select from structure somedocument where ( 1 == 1 ) fields [ str, obj, obj.field1, obj.field2 ];
drop structure somedocument;
select from structure somedocument where ( 1 == 1 ) fields [ str, obj, obj.field1, obj.field2 ];
