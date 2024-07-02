database drop test_db;
database create test_db;
database use test_db;
schema document_schema01 { field1: String; field2: Integer; };
schema document_schema02 { str: String; obj: document_schema01; };
create collection somedocument based on document_schema02;
insert into somedocument objects [  
  { "str": "val", "obj": { "field1": "val11", "field2": "10" } }  
  { "str": "val101", "obj": { "field1": "val31", "field2": "15" } }  
  { "str": "val202", "obj": { "field1": "val22", "field2": "20" } }
];
select from structure somedocument where ( str like val. ) fields [ _rid, str, obj, obj.field1, obj.field2 ];
select from structure somedocument where ( str like val.+ ) fields [ _rid, str, obj, obj.field1, obj.field2 ];
select from structure somedocument where ( str like val.* ) fields [ _rid, str, obj, obj.field1, obj.field2 ];
select from structure somedocument where ( str like \w* ) fields [ _rid, str, obj, obj.field1, obj.field2 ];
select from structure somedocument where ( str like \w* and obj.field1 like val.1 ) fields [ _rid, str, obj, obj.field1, obj.field2 ];

select from structure somedocument where ( str in [ val101, val202 ] ) fields [ _rid, str, obj, obj.field1, obj.field2 ];

select from structure somedocument where ( obj.field2 between 10 and 15 ) fields [ _rid, str, obj, obj.field1, obj.field2 ];
