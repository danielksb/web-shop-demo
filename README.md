# Web Shop in C11

Welcome to the Web Shop in C11 project! This is a personal endeavor designed to deepen understanding in C11, Unix socket programming, C error and memory handling, and system programming in general. Please be aware that this project is purely a hobby undertaking and is neither intended for commercial use nor positioned as a practical open-source application. The primary goal is to foster a hands-on learning experience in low-level programming concepts.

## Build Instructions

To build the project, use the following commands:

```bash
make all   # Build the release version
```

or

```bash
make debug   # Build the debug version with additional debugging information
```

## Database Setup

To set up the PostgreSQL database, you can use Docker Compose. Ensure you have Docker and Docker Compose installed, then run:

```bash
docker-compose up -d
```

This will start the PostgreSQL container in the background.

## Running the Web Shop

To start the server, use the following command:

```bash
./web_shop
```

To start the client, use:

```bash
./client
```

Please note that the server and client communicate using their proprietary protocol, not HTTP.

## Contributions

This project is not intended for commercial use or as an open-source application. It is solely for educational purposes. Contributions and suggestions are welcome but keep in mind the project's learning-focused nature.

## License

This project is not licensed for distribution or commercial use. It is meant for personal learning only.
