database drop FirmaDeCurieratDB;
database create FirmaDeCurieratDB;
database use FirmaDeCurieratDB;

schema Persoana_V0 { nume: String; prenume: String; nr_telefon: String; };
schema Persoana_V1 inherits Persoana_V0 { CNP: String; data_nastere: DateTime; email: String; };
schema Angajat inherits Persoana_V1 { data_angajare: DateTime; salariu: Float; functie: String; };

create table AngajatSQL based on Angajat;

insert into AngajatSQL objects [
    { "nume": "Popescu", "prenume": "Ion", "nr_telefon": "0745123456", "CNP": "1900101123456", "data_nastere": "1991-05-01", "email": "ion.popescu@email.com", "data_angajare": "2015-06-15", "salariu": "5000", "functie": "curier" }
    { "nume": "Ionescu", "prenume": "Maria", "nr_telefon": "0739047820", "CNP": "1910121123456", "data_nastere": "1992-06-01", "email": "ionescu.maria@email.com", "data_angajare": "2015-06-15", "salariu": "4500", "functie": "operator" }
    { "nume": "Georgescu", "prenume": "Vasile", "nr_telefon": "0747902783", "CNP": "9900101123456", "data_nastere": "1993-07-01", "email": "georgescu.vasile@email.com", "data_angajare": "2015-06-15", "salariu": "4000", "functie": "curier" }
    { "nume": "Popa", "prenume": "Teodor", "nr_telefon": "0793947629", "CNP": "2900171123456", "data_nastere": "1994-08-01", "email": "popa.teodor@email.com", "data_angajare": "2015-06-15", "salariu": "7000", "functie": "curier" }
];

select from structure AngajatSQL where ( 1 == 1 ) fields [ _rid, nume, prenume, salariu, functie ];
select from structure AngajatSQL where ( salariu between 4500 and 5000 ) fields [ _rid, nume, prenume, salariu, functie ];
select from structure AngajatSQL where ( salariu between 4500 and 5000 and functie == operator ) fields [ _rid, nume, prenume, salariu, functie ];

select from structure AngajatSQL where ( 1 == 1 ) fields [ _rid, nume, prenume, salariu, functie ];
update structure AngajatSQL where ( functie == curier and salariu between 2000 and 5500 ) modify [ salariu => salariu + salariu * 5 / 100 ];
select from structure AngajatSQL where ( 1 == 1 ) fields [ _rid, nume, prenume, salariu, functie ];

delete from structure AngajatSQL where ( salariu between 4000 and 5000 and functie == curier );
select from structure AngajatSQL where ( 1 == 1 ) fields [ _rid, nume, prenume, salariu, functie ];
