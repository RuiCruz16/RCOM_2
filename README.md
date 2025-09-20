# Project 2 - Computer Networks

## Description
This repository contains the second project for the **Computer Networks** course, carried out in the **first semester of the third year** of the bachelor's degree. The main objective of this project is to deepen knowledge about communication protocols, data transfer, and network application implementation.

## Project Objectives
- Implement a client-server application using sockets.
- Understand the operation of transport protocols such as TCP and UDP.
- Perform file transfers between client and server.
- Analyze and optimize network communication performance.

## Repository Structure
- `clientTCP.c`: Source code for the TCP client.
- `download.c` and `download.h`: Implementation of functionalities related to file downloads.
- `getip.c`: Code for obtaining the IP address.
- Directories `G11_EXP2` to `G11_EXP6` and `TUX3_G11_EXP1`, `TUX4_G11_EXP1`: Contain experiments and tests conducted during the project development.

## Download Process
The download process in this project involves the following steps:

1. **Establishing a Connection**:
   - The client initiates a connection to the server using the TCP protocol.
   - The server listens for incoming connections and accepts the client's request.

2. **Requesting the File**:
   - The client sends a request to the server specifying the file it wants to download.
   - The server verifies the availability of the requested file.

3. **Transferring the File**:
   - The server reads the file in chunks and sends the data to the client.
   - The client receives the data and writes it to a local file.

4. **Closing the Connection**:
   - Once the file transfer is complete, both the client and server close the connection gracefully.

5. **Error Handling**:
   - The implementation includes mechanisms to handle errors such as file not found, connection timeouts, and data corruption during transfer.

## Contributors
- Gonçalo Marques
- Rui Cruz

## License
This project is for educational purposes and is not licensed for commercial use.
