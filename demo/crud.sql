database drop test_db;
database create test_db;
database use test_db;
schema document_schema01 { field1: String; field2: Integer; };
schema document_schema02 { str: String; obj: document_schema01; };

create collection somedocument_test based on document_schema02;
select from structure somedocument_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

insert into somedocument_test objects [  
  { "str": "val1", "obj": { "field1": "val11", "field2": "10" } }  
  { "str": "val2", "obj": { "field1": "val22", "field2": "20" } }  
  { "str": "val3", "obj": { "field1": "val33", "field2": "30" } }  
  { "str": "val4", "obj": { "field1": "val44", "field2": "40" } }  
  { "str": "val5", "obj": { "field1": "val55", "field2": "50" } }  
  { "str": "val6", "obj": { "field1": "val66", "field2": "60" } }  
  { "str": "val7", "obj": { "field1": "val77", "field2": "70" } }  
];
select from structure somedocument_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];

update structure somedocument_test where ( obj.field2 between 30 and 50 ) modify [ obj.field2 => obj.field2 + 5, obj.field1 => val00 ];
select from structure somedocument_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];
select from structure somedocument_test where ( obj.field1 == val00 ) fields [ _rid, str, obj.field1, obj.field2 ];

delete from structure somedocument_test where ( obj.field1 == val00 );
select from structure somedocument_test where ( 1 == 1 ) fields [ _rid, str, obj.field1, obj.field2 ];
