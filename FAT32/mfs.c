#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>


#define MAX_INPUT 255
#define MAX_FILENAME_LENGTH 100
#define MAX_DIR_ENTRIES 16
#define MAX_NAME_LENGTH 12


// FAT32 BPB offsets
#define BPB_BytesPerSec_OFFSET 11
#define BPB_SecPerClus_OFFSET 13
#define BPB_RsvdSecCnt_OFFSET 14
#define BPB_NumFATs_OFFSET 16
#define BPB_FATSz32_OFFSET 36
#define BPB_RootClus_OFFSET 44
#define BPB_ExtFlags_OFFSET 40
#define BPB_FSInfo_OFFSET 48


// File system variables
FILE *fat32_image = NULL;
uint16_t bytes_per_sector;
uint8_t sectors_per_cluster;
uint16_t reserved_sector_count;
uint8_t num_fats;
uint32_t fat_size;
uint16_t ext_flags;
uint32_t root_cluster;
uint32_t fs_info;
uint32_t current_cluster;  // Tracks current directory
char open_filename[MAX_FILENAME_LENGTH + 1];


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct __attribute__((__packed__)) DirectoryEntry
{
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[MAX_DIR_ENTRIES];

int LBAToOffset(uint32_t sector)
{
    return ((sector - 2) * bytes_per_sector) +
           (bytes_per_sector * reserved_sector_count) +
           (num_fats * fat_size * bytes_per_sector);
}

// Reads and initializes the BPB values for the FAT32 filesystem
int16_t LbDir(uint32_t sector) 
{ //Define the function to read and get the next cluster
    uint32_t FAT_address = bytes_per_sector * reserved_sector_count + (sector * 4); //Calculate the FAT address
    int16_t val; //Define a 16 bit integer to store the value
    fseek(fat32_image, FAT_address, SEEK_SET); //Move file pointer to FAT address
    fread(&val, 2, 1, fat32_image); //Read the value from FAT
    return val; //Return the value
}


void BPB()
{
    fseek(fat32_image, BPB_BytesPerSec_OFFSET, SEEK_SET);
    fread(&bytes_per_sector, 2, 1, fat32_image);


    fseek(fat32_image, BPB_SecPerClus_OFFSET, SEEK_SET);
    fread(&sectors_per_cluster, 1, 1, fat32_image);


    fseek(fat32_image, BPB_RsvdSecCnt_OFFSET, SEEK_SET);
    fread(&reserved_sector_count, 2, 1, fat32_image);


    fseek(fat32_image, BPB_NumFATs_OFFSET, SEEK_SET);
    fread(&num_fats, 1, 1, fat32_image);


    fseek(fat32_image, BPB_FATSz32_OFFSET, SEEK_SET);
    fread(&fat_size, 4, 1, fat32_image);


    fseek(fat32_image, BPB_ExtFlags_OFFSET, SEEK_SET);
    fread(&ext_flags, 2, 1, fat32_image);


    fseek(fat32_image, BPB_FSInfo_OFFSET, SEEK_SET);
    fread(&fs_info, 4, 1, fat32_image);


    fseek(fat32_image, BPB_RootClus_OFFSET, SEEK_SET);
    fread(&root_cluster, 4, 1, fat32_image);


    current_cluster = root_cluster;  // Start at the root directory
}

void readDirectory(uint32_t cluster) 

{ //Define the function to read directory entries from a given cluster
    int offset = LBAToOffset(cluster); //Get the offset of the cluster
    fseek(fat32_image, offset, SEEK_SET); //Set the pointer to the offset
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) 
    { //Loop over the maximum directory entries
        fread(&dir[i], sizeof(struct DirectoryEntry), 1, fat32_image); //Read each directory entry into the array

    }
}



void formatfile(char *input, char *formattedName) 
{ //Define the function to format a file name to FAT32 format
    for (int i = 0; i < 11; i++) { formattedName[i] = ' '; } //Initialize formatted name with blank spaces

    char *dot = strchr(input, '.'); //Find the dot in the input
    int nameLength, extLength; //Define variables for name and extension length

    if (dot) { //Check if dot is present
        nameLength = dot - input; //Get name length before dot
        extLength = strlen(dot + 1); //Get extension length
    } else { //If no dot is present
        nameLength = strlen(input); //Set name length to input length
        extLength = 0; //Set extension length to 0
    }

    for (int i = 0; i < nameLength && i < 8; i++) 
    { //Copy name part
        formattedName[i] = input[i];
    }

    if (dot) { //If extension exists
        for (int i = 0; i < extLength && i < 3; i++) 
        { //Copy extension part
            formattedName[8 + i] = dot[1 + i];
        }
    }

    for (int i = 0; i < 11; i++) { //Convert to uppercase
        formattedName[i] = toupper(formattedName[i]);
    }
}


void open_command(char *filename) 
{
    if (fat32_image != NULL) 
    {    // Check if a file system is already open
        printf("Error - File system image already open.\n");
        return;  // Exit if an image is already open
    }

    fat32_image = fopen(filename, "rb+");  // Open file in read-write binary mode
    if (fat32_image == NULL) 
    {  // Check if the file was successfully opened
        printf("Error - Cannot locate file system image.\n");
        return;   // Exit if file could not be opened
    }

    strncpy(open_filename, filename, MAX_FILENAME_LENGTH);// Store the opened filename
    open_filename[MAX_FILENAME_LENGTH] = '\0'; // Ensure null termination for filename
    BPB();  // Call BPB function to initialize file system variables
    readDirectory(root_cluster);   // Read root directory entries into 'dir' array
    printf("File system image '%s' opened successfully.\n", filename);
}


void handle_close()
{
    if (fat32_image == NULL) //check if there is open file system
    {
        printf("Error: File system not open.\n"); //if there is, print this message
        return;
    }
    fclose(fat32_image); //close file system image
    fat32_image = NULL; //reset image to null
    printf("File system closed successfully.\n");
}


void info_command() //display BPB info function
{


    if (fat32_image == NULL) //if file system image is NULL, then print msg saying error
    {
        printf("Error: File system not open.\n");
        return;
    }


    printf("BPB_BytesPerSec: %u (0x%X)\n", bytes_per_sector, bytes_per_sector);
    printf("BPB_SecPerClus: %u (0x%X)\n", sectors_per_cluster, sectors_per_cluster);
    printf("BPB_RsvdSecCnt: %u (0x%X)\n", reserved_sector_count, reserved_sector_count);
    printf("BPB_NumFATs: %u (0x%X)\n", num_fats, num_fats);
    printf("BPB_FATSz32: %u (0x%X)\n", fat_size, fat_size);
    printf("BPB_ExtFlags: %u (0x%X)\n", ext_flags, ext_flags);
    printf("BPB_RootClus: %u (0x%X)\n", root_cluster, root_cluster);
    printf("BPB_FSInfo: %u (0x%X)\n", fs_info, fs_info);

}

void ls_command() // Function to list directory contents in a FAT32 file system
{
    if (fat32_image == NULL) // Check if the file system is open
    {
        printf("Error: File system not open.\n"); // Display error if not open
        return;
    }

    uint32_t currentCluster = current_cluster; // Start with the current cluster
    do
    {
        int offset = LBAToOffset(currentCluster); // Convert cluster to file offset
        fseek(fat32_image, offset, SEEK_SET); // Set file position to the cluster's offset

        for (int i = 0; i < (bytes_per_sector * sectors_per_cluster) / sizeof(struct DirectoryEntry); i++) {
            struct DirectoryEntry entry; // Structure for a directory entry
            fread(&entry, sizeof(struct DirectoryEntry), 1, fat32_image); // Read the directory entry

            if (entry.DIR_Name[0] == 0x00) // Check for end of directory entries
            {
                break; // Exit loop if no more entries
            }

            if (entry.DIR_Name[0] == 0xE5) // Check if the entry is deleted
            {
                continue; // Skip deleted entries
            }

            if (entry.DIR_Attr & 0x08) // Check for volume label
            {
                continue; // Skip volume label entries
            }

            if (entry.DIR_Attr & 0x02 || entry.DIR_Attr & 0x04) // Check for hidden or system attributes
            {
                continue; // Skip hidden or system entries
            }

            char name[MAX_NAME_LENGTH]; // Array to store formatted name
            memset(name, 0, MAX_NAME_LENGTH); // Initialize array to zero
            strncpy(name, entry.DIR_Name, 11); // Copy entry name into array

            for (int j = 0; j < 11; j++) // Process each character in name
            {
                if (name[j] == ' ' || !isprint(name[j])) // Check for space or non-printable character
                {
                    name[j] = '\0'; // Replace with null terminator if found
                    break;
                }
            }

            if (entry.DIR_Attr & 0x10) // Check if entry is a directory
            {
                printf("<DIR> %s\n", name); // Print directory name with "<DIR>" label
            }
            else
            {
                printf("%s\n", name); // Print regular file name
            }
        }

        currentCluster = LbDir(currentCluster); // Move to the next cluster in the directory chain
    }
    while (currentCluster < 0x0FFFFFF8 && currentCluster != 0); // Continue until end of cluster chain
}


void cd_command(char *directory)
{
  
    if (fat32_image == NULL)
    {
        printf("Error: File system not open.\n");
        return;
    }

    if (directory == NULL)
    {
        printf("Error: No directory specified.\n");
        return;
    }

    // Check if the directory is "." (current directory) or ".." (parent directory)
    if (strcmp(directory, ".") == 0)
    {
        printf("Already in current directory.\n");
        return;
    }
    else if (strcmp(directory, "..") == 0)
    {
        if (current_cluster != root_cluster)
        {
            current_cluster = root_cluster; // Adjust to track actual parent clusters if available
            readDirectory(current_cluster);
            printf("Moved up to previous directory.\n");
        }
        else
        {
            printf("Already at root directory.\n");
        }
        return;
    }

    // Format the target directory name to match FAT32 format
    char formattedName[12];
    formatfile(directory, formattedName);

    bool found = false;
    uint32_t newCluster = 0;

    // Search in the current directory for a matching entry with the DIR_Attr indicating a directory
    readDirectory(current_cluster);
    for (int i = 0; i < MAX_DIR_ENTRIES; i++)
    {
        if ((dir[i].DIR_Attr & 0x10) && strncmp(dir[i].DIR_Name, formattedName, 11) == 0)
        {
            found = true;
            // Calculate the new cluster from the high and low values
            newCluster = (dir[i].DIR_FirstClusterHigh << 16) | dir[i].DIR_FirstClusterLow;
            break;
        }
    }

    if (!found) // if not found, print error
    {
        printf("Error: Directory not found.\n");
        return;
    }

    // Update current cluster to the new directory's cluster
    if (newCluster == 0)  // Root directory case
    {
        newCluster = root_cluster;
    }

    readDirectory(newCluster); //read directory function
    current_cluster = newCluster;
    printf("Changed directory: '%s'.\n", directory);

}


void stat_command(char *filename) // Function to display file stats for a specified filename
{
    if (fat32_image == NULL) // Check if file system is open
    {
        printf("Error: File system not open.\n"); // Print error if file system is not open
        return;
    }

    char formattedName[12]; // Array to hold formatted filename
    formatfile(filename, formattedName); // Format the filename to match directory entry format
    bool found = false; // Variable to track if the file is found

    readDirectory(current_cluster); // Read the directory entries from the current cluster
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) // Loop through all directory entries
    {
        if (strncmp(dir[i].DIR_Name, formattedName, 11) == 0) // Compare entry name with formatted filename
        {
            printf("File Attributes: "); // Print file attribute information
            if (dir[i].DIR_Attr & 0x01) // Check if read-only attribute is set
            {
                printf("Read Only ");
            }
            if (dir[i].DIR_Attr & 0x02) // Check if hidden attribute is set
            {
                printf("Hidden ");
            }
            if (dir[i].DIR_Attr & 0x04) // Check if system attribute is set
            {
                printf("System ");
            }
            if (dir[i].DIR_Attr & 0x08) // Check if volume ID attribute is set
            {
                printf("Volume ID ");
            }
            if (dir[i].DIR_Attr & 0x10) // Check if directory attribute is set
            {
                printf("Directory ");
            }
            if (dir[i].DIR_Attr & 0x20) // Check if archive attribute is set
            {
                printf("Archive ");
            }
            printf("\n");

            uint32_t firstClusterHigh = dir[i].DIR_FirstClusterHigh << 16; // Get high part of starting cluster number
            uint32_t firstClusterLow = dir[i].DIR_FirstClusterLow; // Get low part of starting cluster number
            uint32_t firstCluster = firstClusterHigh | firstClusterLow; // Combine high and low parts

            printf("Starting Cluster Number: %u\n", firstCluster); // Print starting cluster number
            if (dir[i].DIR_Attr & 0x10) // Check if entry is a directory
            {
                printf("File Size: 0 bytes\n"); // Directories have size 0 bytes
            }
            else
            {
                printf("File Size: %u bytes\n", dir[i].DIR_FileSize); // Print file size for regular files
            }

            found = true; // Mark file as found
            break; // Exit loop as file is found
        }
    }

    if (!found) // Check if file was not found
    {
        printf("Error: File not found.\n"); // Print error if file is not found
    }
}


void put_command(char *filename, char *new_filename)
{
     printf("dont know how to do put!!");
}


void get_command(char *filename, char *new_filename) // Function to retrieve a file from FAT32 file system
{
    if (fat32_image == NULL) // Check if file system is open
    {
        printf("Error - File system not open.\n"); // Print error if file system is not open
        return;
    }
   
    if (filename == NULL) // Check if a filename is specified
    {
        printf("Error - No file specified.\n"); // Print error if no filename is specified
        return;
    }

    // Prepare the formatted name for the FAT32 directory entry
    char formattedName[12]; // Array to store formatted filename
    formatfile(filename, formattedName); // Format the filename to match FAT32 directory entry format

    // Search for the file in the current directory
    bool found = false; // Flag to track if file is found
    uint32_t firstCluster = 0; // Variable to store starting cluster
    uint32_t fileSize = 0; // Variable to store file size

    readDirectory(current_cluster); // Read directory entries from current cluster
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) // Loop through directory entries
    {
        if (strncmp(dir[i].DIR_Name, formattedName, 11) == 0) // Compare entry name with formatted filename
        {
            found = true; // Mark file as found
            firstCluster = (dir[i].DIR_FirstClusterHigh << 16) | dir[i].DIR_FirstClusterLow; // Get starting cluster
            fileSize = dir[i].DIR_FileSize; // Get file size
            break;
        }
    }

    if (!found) // Check if file was not found
    {
        printf("Error: File not found.\n"); // Print error if file not found
        return;
    }

    // Open the output file for writing
    FILE *outputFile = fopen(new_filename ? new_filename : filename, "wb"); // Open new file to write retrieved data
    if (outputFile == NULL) // Check if file was created successfully
    {
        printf("Error: Unable to create output file.\n"); // Print error if unable to create file
        return;
    }

    // Read the file data from the FAT32 image
    uint32_t cluster = firstCluster; // Start at the first cluster of the file
    uint32_t bytesRemaining = fileSize; // Track bytes remaining to read
    char buffer[bytes_per_sector * sectors_per_cluster]; // Buffer to read file data

    while (bytesRemaining > 0 && cluster < 0x0FFFFFF8) // Loop while there are bytes to read and clusters are valid
    {
        int offset = LBAToOffset(cluster); // Convert cluster to file offset
        fseek(fat32_image, offset, SEEK_SET); // Set file position to cluster's offset

        // Determine how many bytes to read in this cluster
        int bytesToRead = (bytesRemaining < sizeof(buffer)) ? bytesRemaining : sizeof(buffer); // Calculate bytes to read
        fread(buffer, 1, bytesToRead, fat32_image); // Read data into buffer
       
        // Write the data to the output file
        fwrite(buffer, 1, bytesToRead, outputFile); // Write buffer to output file
        bytesRemaining -= bytesToRead; // Decrease bytes remaining

        // Get the next cluster in the chain
        cluster = LbDir(cluster); // Move to next cluster
    }

    fclose(outputFile); // Close the output file
    printf("File '%s' retrieved successfully.\n", filename); // Print success message
}


void read_command(char *filename, int position, int num_bytes, char *option) // Function to read data from a file at a specified position
{
    if (fat32_image == NULL) // Check if file system is open
    {
        printf("Error: File system not open.\n"); // Print error if file system is not open
        return;
    }
   
    if (filename == NULL || num_bytes <= 0 || position < 0) // Validate input parameters
    {
        printf("Error: Invalid parameters.\n"); // Print error if parameters are invalid
        return;
    }

    // Format the filename to match FAT32 format
    char formattedName[12]; // Array to hold formatted filename
    formatfile(filename, formattedName); // Format the filename to match FAT32 directory entry format

    // Locate the file in the directory
    bool found = false; // Flag to track if file is found
    uint32_t firstCluster = 0; // Variable to store starting cluster
    uint32_t fileSize = 0; // Variable to store file size

    readDirectory(current_cluster); // Read directory entries from current cluster
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) // Loop through directory entries
    {
        if (strncmp(dir[i].DIR_Name, formattedName, 11) == 0) // Compare entry name with formatted filename
        {
            found = true; // Mark file as found
            firstCluster = (dir[i].DIR_FirstClusterHigh << 16) | dir[i].DIR_FirstClusterLow; // Get starting cluster
            fileSize = dir[i].DIR_FileSize; // Get file size
            break;
        }
    }

    if (!found) // Check if file was not found
    {
        printf("Error: File not found.\n"); // Print error if file is not found
        return;
    }

    if (position >= fileSize) // Check if position is within file size
    {
        printf("Error: Position is beyond file size.\n"); // Print error if position is out of bounds
        return;
    }

    // Setup for reading
    uint32_t cluster = firstCluster; // Start at the first cluster of the file
    int bytesRead = 0; // Variable to track bytes read
    int clusterOffset = position / (bytes_per_sector * sectors_per_cluster); // Calculate cluster offset for position
    int intraClusterOffset = position % (bytes_per_sector * sectors_per_cluster); // Calculate intra-cluster offset

    // Skip clusters until reaching the specified position
    while (clusterOffset-- > 0 && cluster < 0x0FFFFFF8) // Loop to skip clusters
    {
        cluster = LbDir(cluster); // Move to next cluster
    }

    // Position the file pointer at the correct offset
    fseek(fat32_image, LBAToOffset(cluster) + intraClusterOffset, SEEK_SET); // Set position to read from

    int remainingBytes = num_bytes; // Track number of bytes to read
    char buffer[remainingBytes]; // Buffer to store read data

    // Read the requested bytes from the file
    bytesRead = fread(buffer, 1, remainingBytes, fat32_image); // Read data into buffer

    // Output based on the option
    printf("Read data: ");
    for (int i = 0; i < bytesRead; i++) // Loop through read bytes for output
    {
        if (option != NULL && strcmp(option, "-ascii") == 0) // Check for ASCII option
        {
            printf("%c", isprint(buffer[i]) ? buffer[i] : '.'); // Print ASCII characters or dot for non-printable
        }
        else if (option != NULL && strcmp(option, "-dec") == 0) // Check for decimal option
        {
            printf("%d ", (unsigned char)buffer[i]); // Print decimal value of byte
        }
        else // Default to hexadecimal output
        {
            printf("0x%02X ", (unsigned char)buffer[i]); // Print hexadecimal value of byte
        }
    }
    printf("\n");
}


void delete_command(char *filename) // Function to delete a file in FAT32 file system
{
    if (fat32_image == NULL) // Check if file system is open
    {
        printf("Error: File system not open.\n"); // Print error if file system is not open
        return;
    }

    char formattedName[12]; // Array to hold formatted filename
    formatfile(filename, formattedName); // Format the filename to match FAT32 directory entry format
    bool found = false; // Flag to track if file is found

    readDirectory(current_cluster); // Read directory entries from current cluster
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) // Loop through directory entries
    {
        if (strncmp(dir[i].DIR_Name, formattedName, 11) == 0) // Compare entry name with formatted filename
        {
            dir[i].DIR_Name[0] = 0xE5; // Mark file as deleted by setting first byte to 0xE5
            int offset = LBAToOffset(current_cluster) + (i * sizeof(struct DirectoryEntry)); // Calculate offset of entry
            fseek(fat32_image, offset, SEEK_SET); // Set file position to directory entry offset
            fwrite(&dir[i], sizeof(struct DirectoryEntry), 1, fat32_image); // Write updated entry back to file
            found = true; // Mark file as found
            printf("File '%s' marked as deleted.\n", filename); // Print success message
            break;
        }
    }

    if (!found) // Check if file was not found
    {
        printf("Error: File not found.\n"); // Print error if file is not found
    }
}


void undelete_command(char *filename) // Function to undelete a file in FAT32 file system
{
    if (fat32_image == NULL) // Check if file system is open
    {
        printf("Error: File system not open.\n"); // Print error if file system is not open
        return;
    }

    // Format the filename to match FAT32's 11-character format
    char formattedName[12]; // Array to hold formatted filename
    formatfile(filename, formattedName); // Format the filename to match FAT32 directory entry format

    // Set up the formatted name to match deleted files (0xE5 at the start)
    char searchName[12]; // Array to hold search pattern for deleted file
    searchName[0] = 0xE5; // Deleted marker
    memcpy(&searchName[1], &formattedName[1], 10); // Copy rest of formatted name

    bool found = false; // Flag to track if file is found
    readDirectory(current_cluster); // Read directory entries from current cluster
    
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) // Loop through directory entries
    {
        // Match entries with the deleted marker and rest of the name intact
        if (strncmp(dir[i].DIR_Name, searchName, 11) == 0) // Check for deleted file match
        {
            // Restore the first character of the filename
            dir[i].DIR_Name[0] = formattedName[0]; // Set first character to restore name
            
            // Write changes back to the disk
            int offset = LBAToOffset(current_cluster) + (i * sizeof(struct DirectoryEntry)); // Calculate entry offset
            fseek(fat32_image, offset, SEEK_SET); // Set file position to directory entry offset
            fwrite(&dir[i], sizeof(struct DirectoryEntry), 1, fat32_image); // Write updated entry to file
            
            found = true; // Mark file as found
            printf("File '%s' undeleted successfully.\n", filename); // Print success message
            break;
        }
    }

    if (!found) // Check if deleted file was not found
    {
        printf("Error: Deleted file not found.\n"); // Print error 
    }
}


void quit_command() //fclose image
{
    if (fat32_image != NULL)
    {
        fclose(fat32_image);
    }
    exit(0);
}


int main() // Main function to handle user commands in the FAT32 file system
{
    char input[MAX_INPUT]; // Buffer to hold user input
    char *command; // Pointer to store command part of input
    char *arg1; // Pointer for first argument
    char *arg2; // Pointer for second argument
    char *arg3; // Pointer for third argument
    char *arg4; // Pointer for fourth argument

    while (true) // Infinite loop for command processing
    {
        printf("mfs> "); // Prompt for user input

        if (fgets(input, MAX_INPUT, stdin) == NULL) // Read input from user
        {
            break; // Exit loop if input fails
        }

        input[strcspn(input, "\n")] = 0; // Remove newline character from input

        command = strtok(input, " "); // Tokenize input to get command
        arg1 = strtok(NULL, " "); // Tokenize to get first argument
        arg2 = strtok(NULL, " "); // Tokenize to get second argument
        arg3 = strtok(NULL, " "); // Tokenize to get third argument
        arg4 = strtok(NULL, " "); // Tokenize to get fourth argument

        if (command == NULL) 
        {
            continue; 
        }

        for (char *p = command; *p; ++p) *p = tolower(*p); 

        if (strcmp(command, "open") == 0) 
        {
            open_command(arg1);
        }
        else if (strcmp(command, "close") == 0)
        {
            quit_command(); 
        }
        else if (strcmp(command, "info") == 0) 
        {
            info_command(); 
        }
        else if (strcmp(command, "ls") == 0) 
        {
            ls_command(); 
        }
        else if (strcmp(command, "stat") == 0) 
        {
            stat_command(arg1);
        }
        else if (strcmp(command, "cd") == 0) 
        {
            cd_command(arg1); 
        }
        else if (strcmp(command, "del") == 0) 
        {
            delete_command(arg1); 
        }
        else if (strcmp(command, "put") == 0) 
        {
            put_command(arg1, arg2); 
        }
        else if (strcmp(command, "get") == 0) 
        {
            get_command(arg1, arg2); 
        }
        else if (strcmp(command, "read") == 0) 
        {
            read_command(arg1, atoi(arg2), atoi(arg3), arg4); 
        }
        else if (strcmp(command, "undel") == 0) 
        {
            undelete_command(arg1); 
        }
        else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) 
        {
            quit_command(); 
        }
        else 
        {
            printf("Error: Unknown command '%s'.\n", command); // Print error for unknown command
        }
    }

    return 0; // Exit program
}
