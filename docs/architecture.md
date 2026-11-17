# Connection Architecture

Options:
- P2P
- Client - Server

For the core of the architecture, the chosen connection model is P2P. In practice, though, it will let the user interact in a "Hybrid" way.
That is, the system will work as P2P, and if the user wishes, they can share their files across all their devices, which allows having a copy on each one, but with a fully synchronized tree. Alternatively, the user can also interact in a "Client - Server" manner simulated on top of the P2P architecture; that is, a single device can act as a pivot where all the files are stored, but at any time these files can be synced to another device, or only selected ones can be downloaded. Naturally, the pivot computer needs to be powered on for this to work.

- Discovery: mDNS.
- Pairing: QR.
### Connection Type
Options:
- LAN
- Internet
For the first version of the system, only LAN connection will be considered, since it is faster and cheaper to develop.

# Conflict Resolution

Options:
- LWW
- Vector Clocks
- CRDTs

LWW will be used for this project because it is the easiest approach to implement for the first version and, moreover, conflict management and versioning are not the priority of this project right now. However, in a future version, implementing a more sophisticated method for conflict management is not ruled out.

# Change Detection

Options:
- Polling
- File-Watching

This project will use File-Watching for change detection. The reason is simple: the system needs to run on multiple devices, and on some of them the CPU cost of Polling can be harmful, for example on battery-dependent devices such as phones or laptops.

# Transfer

This project will use the Rsync Algorithm to transmit and synchronize the user's files over the network, which ensures correct and efficient operation of the synchronization engine.

# Encryption

Encryption in transit will be used to transfer files securely. Encryption at rest, on the other hand, will be entirely optional and at the user's discretion, which allows the system to be flexible for all types of users.

- Encryption at rest: A DEK/KEK system is used.

# Technology

- C++
This will be the technology used for this project. Although other options were analyzed, it was chosen for completely personal reasons; even so, it is a fully suitable technology for the system.

# Example

Discovery → Pairing → Change detection → Sync engine → Conflicts / Encryption / Backups