database create FirmaDeCurieratBD;
database use FirmaDeCurieratBD;

schema Persoana_V0 { nume: String; prenume: String; nr_telefon: String; };

schema Persoana_V1 inherits Persoana_V0 { CNP: String; data_nastere: DateTime; email: String; };

schema Angajat inherits Persoana_V1 { data_angajare: DateTime; salariu: Float; functie: String; };

schema Client inherits Persoana_V1 {};

schema PersoanaDeContact inherits Persoana_V0 {};

schema Adresa { judet: String; oras: String; detalii: String; };

schema Colet { curier: Angajat; client: Client; persoana_contact: PersoanaDeContact; adresa_livrare: Adresa; adresa_facturare: Adresa; greutate_grame: Integer; pret_expediere: Float; };

schema SedintaSuport { operator: Angajat; client: Client; data: DateTime; descriere: String; };

create collections from [ Colet, SedintaSuport ] using format "%TypeName%NOSQL";
create tables from [ Angajat, Client, Adresa ] using format "%TypeName%SQL";
create tables from [ Colet, SedintaSuport ] using format "%TypeName%_TEST_SQL";
