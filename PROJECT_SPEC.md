# Event Reservation Platform — Project Specification (Saved Reference)

Date: 12 Dec 2025

This file stores the project specification provided by the user for quick reference during development. It summarizes the key requirements and protocols for both the User application and the Event-reservation Server (ES).

## Overview
Implement an event reservation platform with:
- Event-reservation Server (ES) running on a known IP/port
- Multiple User application instances (clients) over Internet
- Both UDP and TCP protocols per specified operations

Users can:
- Login/register, logout, unregister, change password
- Create, close, list own events (myevents), list all events
- Show event details incl. file, reserve seats
- List own reservations (latest 50)

## Invocation
- User: `./user [-n ESIP] [-p ESport]`
  - ESIP: IP of ES (optional; default: localhost)
  - ESport: well-known port for ES (optional; default: 58000+GN)
- ES: `./ES [-p ESport] [-v]`
  - ESport: UDP and TCP well-known port (optional; default: 58000+GN)
  - -v: verbose mode (logs UID, request type, origin IP/port)

## User Commands
- `login UID password` (UDP)
- `changePass oldPassword newPassword` (TCP)
- `unregister` (UDP)
- `logout` (UDP)
- `exit` (local only; if logged in, request logout first)
- `create name event_fname event_date num_attendees` (TCP)
- `close EID` (TCP)
- `myevents` or `mye` (UDP)
- `list` (TCP)
- `show EID` (TCP)
- `reserve EID value` (TCP)
- `myreservations` or `myr` (UDP)

Only one messaging command at a time; display all results in human-friendly format.

## Protocols
UID: 6 digits; EID: 3 digits; items separated by single space; messages end with `\n`. If syntax invalid or values invalid: status = `ERR`. Unexpected message → reply `ERR`.

### UDP: User–ES
Requests and replies:
- `LIN UID password` → `RLI status`
  - Existing user & correct password → `OK`
  - Incorrect password → `NOK`
  - New user registered → `REG`
- `LOU UID password` → `RLO status`
  - Logged in → `OK` and then logged out
  - Not logged in → `NOK`
  - Not registered → `UNR`
  - Wrong password → `WRP`
- `UNR UID password` → `RUR status`
  - Registered & logged in → `OK` and unregister
  - Not logged in → `NOK`
  - Not registered → `UNR`
  - Wrong password → `WRP`
- `LME UID password` → `RME status [EID state]*`
  - Not logged in → `NLG`
  - No events → `NOK`
  - Otherwise `OK` + pairs of `EID state`
  - state: 1=open; 0=past; 2=future sold out; 3=closed by owner
  - Wrong password → `WRP`
- `LMR UID password` → `RMR status [EID date value]*`
  - Not logged in → `NLG`
  - No reservations → `NOK`
  - Otherwise `OK` + up to 50 entries of `EID dd-mm-yyyy hh:mm:ss value`
  - Wrong password → `WRP`

### TCP: Messaging/File transfer
- `CRE UID password name event_date attendance_size Fname Fsize Fdata` → `RCE status [EID]`
  - Not logged in → `NLG`; wrong password → `WRP`
  - On success → `OK EID` and store server-side copy of `Fname`
- `CLS UID password EID` → `RCL status`
  - Success (owner, open) → `OK`
  - User issues: `NLG`, `NOK` (incorrect password), `NID` (user not exists)
  - Event issues: `NOE` (no event), `EOW` (not owner), `SLD` (sold out), `PST` (past), `CLO` (already closed)
- `LST` → `RLS status [EID name state event_date]*` ending with `\n`
  - No events → `NOK`; otherwise `OK` followed by list until terminal `\n`
  - state: 1=future not sold out; 0=past; 2=future sold out; 3=closed
- `SED EID` → `RSE status [UID name event_date attendance_size Seats_reserved Fname Fsize Fdata]`
  - On success `OK` + metadata and file content; otherwise `NOK`
- `RID UID password EID people` → `RRI status [n_seats]*`
  - Not logged in → `NLG`; password wrong → `WRP`
  - Event closed → `CLS`; sold out → `SLD`; past → `PST`
  - Accepted → `ACC`
  - Rejected (too many seats) → `REJ n_seats`
- `CPS UID oldPassword newPassword` → `RCP status`
  - Not logged in → `NLG`; wrong oldPassword → `NOK`; user not exists → `NID`
  - Success → `OK`

## Constraints
- Filenames: up to 24 chars [A-Za-z0-9_.-], with extension `nnn…nnnn.xxx`
- File size: ≤ 10 MB (10^7 B), up to 8 digits
- Attendance size: 10–999; reservation people: 1–999

## Implementation Notes
- Use C/C++ with system calls: fgets, sscanf/sprintf, sockets (UDP/TCP), sendto/recvfrom, write/read, select.
- read()/write() may return fewer bytes; handle partial I/O robustly.
- Handle protocol errors gracefully; no abrupt termination.
- Display replies in human-friendly format.

## Next Steps
- Define data structures for users, events, and reservations
- Implement UDP server/client handlers for user management and listings
- Implement TCP server/client handlers for events, file transfer, and reservations
- Build Makefiles and test harnesses
- Add validation utilities (UID, EID, date parsing, filename constraints, etc.)
