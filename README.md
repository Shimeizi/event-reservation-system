# 🎟️ Event Reservation System

A project developed for the **Computer Networks** course.

## 📖 Overview

This project implements an event reservation platform based on a client-server architecture.

The system allows users to create, manage, browse, and reserve events through a custom application-layer protocol built on top of TCP and UDP sockets.

The platform consists of two components:

* 🖥️ Event Reservation Server (ES)
* 👤 User Application (User)

## ✨ Features

* 🔐 User registration and authentication
* 🎟️ Event creation and management
* 🔒 Event closing and status tracking
* 📋 Event listing and visualization
* 🪑 Seat reservation management
* 📂 Event file transfer
* 🌐 TCP and UDP communication protocols
* 🧵 Concurrent client-server interactions

## 🛠️ Built With

* C / C++
* BSD Sockets
* TCP
* UDP

## ⚙️ Compilation

Compile the project using:

```bash
make
```

## ▶️ Running

### Start the Server

```bash
./ES -p 58000 -v
```

### Start the Client

```bash
./user -n <server_ip> -p 58000
```

## 📡 Supported Commands

### User Management

* `login`
* `logout`
* `changePass`
* `unregister`

### Event Management

* `create`
* `close`
* `myevents`

### Event Reservation

* `list`
* `show`
* `reserve`
* `myreservations`

## 📄 Additional Information

For a detailed description of the project requirements, please refer to the project specification PDFs included in this repository.
