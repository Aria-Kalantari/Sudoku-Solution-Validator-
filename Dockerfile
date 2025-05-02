FROM gcc:latest

# Create working directory inside container
WORKDIR /app

# Copy the source file
COPY sudoku_validator.c .

# Compile the C file
RUN gcc -o sudoku_validator sudoku_validator.c -pthread

# Default command: run the validator
CMD ["./sudoku_validator"]
