--
-- Upgrade an existing SMSD database to support latest features.
-- (it is not necessary to run this script on a new database)
--

--
-- update inbox and outbox coding constraints to include entries for 7bit ASCII, 7bit GSM and Network Default
--
alter table inbox drop constraint "inbox_Coding_check";
alter table inbox add constraint "inbox_Coding_check"
  check ("Coding" IN ('Default_No_Compression', 'Unicode_No_Compression', '8bit', 'Default_Compression', 'Unicode_Compression', 'ASCII'));

alter table outbox drop constraint "outbox_Coding_check";
alter table outbox add constraint "outbox_Coding_check"
  check ("Coding" IN ('Network_Default', 'Default_No_Compression', 'Unicode_No_Compression', '8bit', 'Default_Compression', 'Unicode_Compression', 'ASCII'));

alter table outbox_multipart drop constraint "outbox_multipart_Coding_check";
alter table outbox_multipart add constraint "outbox_multipart_Coding_check"
  check ("Coding" IN ('Network_Default', 'Default_No_Compression', 'Unicode_No_Compression', '8bit', 'Default_Compression', 'Unicode_Compression', 'ASCII'));

alter table sentitems drop constraint "sentitems_Coding_check";
alter table sentitems add constraint "sentitems_Coding_check"
  check ("Coding" IN ('Default_No_Compression', 'Unicode_No_Compression', '8bit', 'Default_Compression', 'Unicode_Compression', 'ASCII'));
