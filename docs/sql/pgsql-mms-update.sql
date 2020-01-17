drop table inbox_mms_parts;

create table inbox_mms_parts
(
    "ID" serial not null primary key,
    "INBOX_ID" integer references inbox("ID"),
    "Headers" text,
    "MediaType" varchar(255),
    "Data" bytea
);

drop table outbox_mms_parts;

create table outbox_mms_parts
(
    "ID" serial not null primary key,
    "OUTBOX_ID" integer references outbox("ID"),
    "Headers" text,
    "MediaType" varchar(255),
    "Data" bytea
);