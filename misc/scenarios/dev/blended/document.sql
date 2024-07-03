database drop test_db;
database create test_db;
database use test_db;
schema document_schema01 { field1: String; field2: Integer; };
schema document_schema02 { str: String; obj: document_schema01; };

create collection somedocument_test based on document_schema02 blended;

select from structure somedocument_test where ( 1 == 1 ) fields [ _rid, str, field1, field2 ];

insert into somedocument_test objects [  
  { "str": "val11", "field1": "val11", "field2": "11" } 
  { "str": "val111", "field1": "val111", "field2": "111" }
  { "str": "val111", "field1": "val111", "field2": "111" }
  { "str": "val99",  "field1": "val99", "field2": "99" }  
];

select from structure somedocument_test where ( 1 == 1 ) fields [ _rid, str, field1, field2 ];
