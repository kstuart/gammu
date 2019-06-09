--
-- Upgrade an existing SMSD database to support CDMA features.
-- (it is not necessary to run this script on a new database)
--

--
-- update inbox and outbox coding constraints to include support for 7bit ASCII
--
alter table inbox drop constraint "inbox_Coding_check";
alter table inbox add constraint "inbox_Coding_check"
  check ("Coding" IN ('Default_No_Compression', 'Unicode_No_Compression', '8bit', 'Default_Compression', 'Unicode_Compression', '7bit_ASCII'));

alter table outbox drop constraint "outbox_Coding_check";
alter table outbox add constraint "outbox_Coding_check"
  check ("Coding" IN ('Default_No_Compression', 'Unicode_No_Compression', '8bit', 'Default_Compression', 'Unicode_Compression', '7bit_ASCII'));

