#![allow(non_snake_case)]
#![allow(static_mut_refs)]

use serde::Deserialize; 
use std::env;
use std::fs::{self, OpenOptions};
use std::io::{Seek, SeekFrom, Write};
use std::process;

const SECTOR_SIZE: u64 = 512;
const BOOT_PARTITION_LBA: u64 = 2048;
static mut BOOTLOADER: Option<String> = None;

#[derive(Deserialize, Debug)]
struct ImageConfig {
    entries: Vec<Entry>,
}

#[derive(Deserialize, Debug)]
struct Entry {
    #[serde(rename = "operation")]
    operation: String,
    flags: Vec<serde_json::Value>,
    #[serde(default)]
    extraflags: Option<Vec<serde_json::Value>>,
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        eprintln!("Syntax: {} <image.json> <disk image>", args[0]);
        std::process::exit(1);
    }

    let filename = &args[1];
    let arg1 = &args[2];

    let file_content = fs::read_to_string(filename).expect("Failed to read file");
    let config: ImageConfig = serde_json::from_str(&file_content).expect("Invalid JSON format");

    for (index, entry) in config.entries.iter().enumerate() {
        match entry.operation.as_str() {
            "createdisk" => CreateDisk(&entry.flags, arg1),
            "diskpart" => DiskPart(&entry.flags, &entry.extraflags, arg1),
            "fat" => Fat(&entry.flags, &entry.extraflags, arg1),
            "imageScript" => ImageScript(&entry.flags, &entry.extraflags, arg1),
            other => {
                eprintln!("Entry {}: Unknown operation {}", index + 1, other);
                process::exit(1);
            }
        }
    }
}

fn CreateDisk(flags: &[serde_json::Value], arg1: &str) {
    let mut cmdArgs = vec![];
    let mut input = None;
    let mut output = None;
    let mut bps = None;
    let mut count = None;

    for pair in flags.chunks(2) {
        if let [keyVal, valVal] = pair {
            match(keyVal.as_str(), valVal.as_str()) {
                (Some("input"), Some(v)) => input = Some(v.to_string()),
                (Some("output"), Some(v)) => output = Some(v.replace("$ARG1", arg1)),
                _ => {
                    if keyVal == "bps" {
                        bps = valVal.as_u64();
                    }
                    else if keyVal == "count" {
                        count = valVal.as_u64();
                    }
                }
            }
        } else {
            eprintln!("Malformed flags pair : {:?}", pair);
            process::exit(1);
        }
    }

    let input = input.unwrap_or_else(|| {
        eprintln!("CreateDisk: missing input file");
        process::exit(1);
    });

    let output = output.unwrap_or_else(|| {
        eprintln!("CreateDisk: missing output file");
        process::exit(1);
    });
    
    cmdArgs.push(format!("if={}", input));
    cmdArgs.push(format!("of={}", output));
    cmdArgs.push(format!("bs={}", bps.unwrap_or(512)));
    cmdArgs.push(format!("count={}", count.unwrap_or(2880)));

    println!("Creating disk {}, sectors: {}, bps: {}", output, count.unwrap_or(2880), bps.unwrap_or(512));

    if let Err(e) = duct::cmd("dd", cmdArgs).stdout_null().stderr_null().run() {
        eprintln!("Failed to create DISK {}", e);
        process::exit(1);
    }
}

fn DiskPart(
    flags: &[serde_json::Value],
    extraflags: &Option<Vec<serde_json::Value>>,
    arg1: &str,
) {
    let mut CmdArgs = vec!["-s".to_string()];
    let mut subOperation: Option<String> = None;
    let mut inputFile: Option<String> = None;

    for pair in flags.chunks(2) {
        if let [keyVal, valVal] = pair {
            match (keyVal.as_str(), valVal.as_str()) {
                (Some("input"), Some(v)) => inputFile = Some(v.replace("$ARG1", arg1)),
                (Some("operation"), Some(v)) => subOperation = Some(v.to_string()),
                _ => {}
            }
        }
    }

    let subOperation = subOperation.unwrap_or_else(|| {
        eprintln!("DiskPart: missing subOperation");
        process::exit(1);
    });

    let inputFile = inputFile.unwrap_or_else(|| {
        eprintln!("DiskPart: missing input file");
        process::exit(1);
    });

    CmdArgs.push(inputFile.clone());

    match subOperation.as_str() {
        "mklabel" => {
            CmdArgs.push("mklabel".to_string());
            if let Some(extra) = extraflags {
                if let [key, val] = &extra[..] {
                    if key.as_str() == Some("partSchemeType") {
                        CmdArgs.push(val.as_str().unwrap_or("").to_string());
                        println!("Creating MBR on disk {}", inputFile);
                    }
                }
            }
        }
        "mkpart" => {
            CmdArgs.push("mkpart".to_string());

            let mut partType = None;
            let mut startLba = None;
            let mut sectorCount = None;
            
            if let Some(extra) = extraflags {
                for pair in extra.chunks(2) {
                    if let [key, val] = pair {
                        match key.as_str() {
                            Some("partType") => {
                                partType = val.as_str().map(|s| s.to_string());
                            }
                            Some("partStartLba") => {
                                if let Some(n) = val.as_u64() {
                                    startLba = Some(format!("{}s", n));
                                }
                            }
                            Some("partSectors") => {
                                if let Some(n) = val.as_u64() {
                                    sectorCount = Some(format!("{}s", n));
                                }
                            }
                            _ => {}
                        }
                    }
                }
            }

            if let (Some(p), Some(start), Some(size)) = (partType, startLba, sectorCount) {
                CmdArgs.push(p.clone());
                CmdArgs.push(start.clone());
                CmdArgs.push(size.clone());
                println!("Creating partition on disk {}, type: {}, start LBA: {}, sectors: {}", inputFile, p, start, size);
            } else {
                eprintln!("DiskPart: subOperation mkpart requires partition type, partition start LBA & partition size in sectors");
                process::exit(1);
            }
        }
        "set" => {
            CmdArgs.push("set".to_string());

            let mut index = None;
            let mut flag = None;
            let mut value = None;
            
            if let Some(extra) = extraflags {
                for pair in extra.chunks(2) {
                    if let [key, val] = pair {
                        match key.as_str() {
                            Some("partitionIndex") => {
                                if let Some(n) = val.as_u64() {
                                    index = Some(n.to_string());
                                }
                            }
                            Some("variable") => {
                                flag = val.as_str().map(|s| s.to_string());
                            }
                            Some("value") => {
                                value = val.as_str().map(|s| s.to_string());
                            }
                            _ => {}
                        }
                    }
                }
            }

            if let (Some(i), Some(f), Some(v)) = (index, flag, value) {
                CmdArgs.push(i.clone());
                CmdArgs.push(f.clone());
                CmdArgs.push(v.clone());
                println!("Setting partition {} data on disk {}, var: {}, val: {}", i, inputFile, f, v);
            } else {
                eprintln!("DiskPart: subOperation set requires partition index, variable & value");
                process::exit(1);
            }
        }
        _ => {
            eprintln!("DiskPart: Unknown operation {}", subOperation);
            process::exit(1);
        }
    }

    if let Err(e) = duct::cmd("parted", CmdArgs).run() {
        eprintln!("Failed to partition DISK: {}", e);
        process::exit(1);
    }
}

fn Fat(flags: &[serde_json::Value], extraflags: &Option<Vec<serde_json::Value>>, arg1: &str) {
    let mut CmdArgs = vec!["-s".to_string()];
    let mut subOperation = None;
    let mut inputFile = None;

    for pair in flags.chunks(2) {
        if let [key, val] = pair {
            match(key.as_str(), val.as_str()) {
                (Some("input"), Some(v)) => inputFile = Some(v.replace("$ARG1", arg1)),
                (Some("operation"), Some(v)) => subOperation = Some(v.to_string()),
                (Some("partitionIndex"), Some(v)) => {
                    CmdArgs.push("-p".to_string());
                    CmdArgs.push(v.to_string());
                }
                _ => {}
            }
        }
    }

    let inputFile = inputFile.unwrap_or_else(|| {
        eprintln!("FAT: Missing input file");
        process::exit(1);
    });

    CmdArgs.push(inputFile.clone());

    let subOperation = subOperation.unwrap_or_else(|| {
        eprintln!("FAT: missing subOperation");
        process::exit(1);
    });

    CmdArgs.push("--operation".to_string());
    CmdArgs.push(subOperation.clone());

    match subOperation.as_str() {
        "createfile" | "mkdir" => {
            if let Some(extra) = extraflags {
                if extra.len() >= 2 {
                    CmdArgs.push("--input".to_string());
                    CmdArgs.push(extra[1].as_str().unwrap_or("").to_string());
                    println!("Creating File / Directory on disk {}, path: {}", inputFile, extra[1].as_str().unwrap_or("").to_string());
                } else {
                    eprintln!("FAT: Not enough extraflags for {}", subOperation);   
                    process::exit(1);
                }
            }
        }
        "diskcpy" => {
            if let Some(extra) = extraflags {
                if extra.len() >= 4 {
                    CmdArgs.push("--input".to_string());
                    CmdArgs.push(extra[1].as_str().unwrap_or("").to_string());
                    CmdArgs.push("--output".to_string());
                    CmdArgs.push(extra[3].as_str().unwrap_or("").to_string());
                    println!("Copying file to disk {}, path: {}, path in disk: {}", inputFile, extra[1].as_str().unwrap_or("").to_string(), extra[3].as_str().unwrap_or("").to_string());
                } else {
                    eprintln!("FAT: Not enough extraflags for diskcpy");
                    process::exit(1);
                }
            }   
        }
        "mkfs" => {
            if let Some(extra) = extraflags {
                for pair in extra.chunks(2) {
                    if let [key, val] = pair {
                        if let (Some(k), Some(v)) = (key.as_str(), val.as_str()) {
                            let flag = match k {
                                "bytespersector" => Some("-b"),
                                "sectorspercluster" => Some("-c"),
                                "mediadesctype" => Some("-m"),
                                "sectorspertrack" => Some("-t"),
                                "headcount" => Some("-h"),
                                "drivenumber" => Some("-d"),
                                "volumeid" => Some("-v"),
                                "volumelabel" => Some("-l"),
                                _ => None,
                            };
                            if let Some(f) = flag {
                                CmdArgs.push(f.to_string());
                                CmdArgs.push(v.to_string());
                            } else {
                                eprintln!("FAT: Unknown mkfs extraflag {}", k);
                                process::exit(1);
                            }
                        }
                    }
                }
                println!("Formatting disk {}", inputFile);
            }
        }
        _ => {
            eprintln!("FAT: Unknown operation {}", subOperation);
            process::exit(1);
        }
    }

    if let Err(e) = duct::cmd("output/tools/fat", CmdArgs).run() {
        eprintln!("Failed to run FAT: {}", e);
        process::exit(1);
    }
}

fn ImageScript(
    flags: &[serde_json::Value],
    extraflags: &Option<Vec<serde_json::Value>>,
    arg1: &String,
) {
    let mut inputFile = None;
    let mut subOperation = None;

    for pair in flags.chunks(2) {
        if let [key, val] = pair {
            match key.as_str() {
                Some("input") => {
                    inputFile = Some(val.as_str().unwrap_or("").replace("$ARG1", arg1));
                }
                Some("operation") => {
                    subOperation = Some(val.as_str().unwrap_or("").to_string());
                }
                _ => {}
            }
        }
    }

    let inputFile = inputFile.unwrap_or_else(|| {
        eprintln!("ImageScript: missing input file");
        process::exit(1);
    });

    let subOperation = subOperation.unwrap_or_else(|| {
        eprintln!("ImageScript: missing subOperation");
        process::exit(1);
    });

    match subOperation.as_str() {
        "setBootFile" => {
            let Some(extra) = extraflags else {
                eprintln!("ImageScript setBootFile: missing extraflags");
                process::exit(1);
            };

            if extra.len() < 2 {
                eprintln!("ImageScript setBootFile: malformed extraflags");
                process::exit(1);
            }

            let bootFilePath = extra[1].as_str().unwrap_or("");

            println!("Setting boot file on disk {}: {}", inputFile, bootFilePath);

            let CmdArgs = vec![
                format!("if={}", bootFilePath),
                format!("of={}", inputFile),
                "conv=notrunc".to_string(),
                "bs=1".to_string(),
                "count=3".to_string(),
                format!("seek={}", BOOT_PARTITION_LBA * SECTOR_SIZE).to_string(),
            ];

            if let Err(e) = duct::cmd("dd", &CmdArgs).stdout_null().stderr_null().run() {
                eprintln!("ImageScript setBootFile: failed to run dd: {}", e);
                process::exit(1);
            }

            let CmdArgs1 = vec![
                format!("if={}", bootFilePath),
                format!("of={}", inputFile),
                "conv=notrunc".to_string(),
                "bs=1".to_string(),
                format!("seek={}", 90 + BOOT_PARTITION_LBA * SECTOR_SIZE).to_string(),
                "skip=90".to_string(),
            ];

            if let Err(e) = duct::cmd("dd", &CmdArgs1).stdout_null().stderr_null().run() {
                eprintln!("ImageScript setBootFile: failed to run dd: {}", e);
                process::exit(1);
            }

            let bootloaderPath = unsafe {
                BOOTLOADER.as_ref().unwrap_or_else(|| {
                    eprintln!("ImageScript setBootFile: bootloader path is unspecified, please specify ImageScript setBootloader before ImageScript setBootFile");
                    process::exit(1);
                })
            };

            let metadata = fs::metadata(bootloaderPath).unwrap_or_else(|e| {
                eprintln!("ImageScript setBootFile: failed to get bootloader metadata: {}", e);
                process::exit(1);
            });

            let bootloaderSize = metadata.len();
            let bootloaderSectors = (bootloaderSize + (SECTOR_SIZE - 1)) / SECTOR_SIZE;

            let mut file = OpenOptions::new().read(true).write(true).open(&inputFile).unwrap_or_else(|e| {
                eprintln!("ImageScript setBootFile: failed to open image file {}: {}", inputFile, e);
                process::exit(1);
            });

            let sectorOffset = BOOT_PARTITION_LBA * SECTOR_SIZE + 484;
            file.seek(SeekFrom::Start(sectorOffset)).unwrap_or_else(|e| {
                eprintln!("ImageScript setBootFile: failed to seek {}: {}", inputFile, e);
                process::exit(1);
            });

            let sectorBytes = (bootloaderSectors as u32).to_le_bytes();
            file.write_all(&sectorBytes).unwrap_or_else(|e| {
                eprintln!("ImageScript setBootFile: failed to write bootloader sector count: {}", e);
                process::exit(1);
            });
        }
        "setBootloader" => {
            let Some(extra) = extraflags else {
                eprintln!("ImageScript setBootloader: missing extraflags");
                process::exit(1);
            };


            if extra.len() < 2 {
                eprintln!("ImageScript setBootloader: malformed extraflags");
                process::exit(1);
            };

            let bootloaderPath = extra[1].as_str().unwrap_or("");

            println!("Setting bootloader on disk {}: {}", inputFile, bootloaderPath);

            let cmdArgs = vec![
                format!("if={}", bootloaderPath),
                format!("of={}", inputFile),
                "conv=notrunc".to_string(),
                "bs=512".to_string(),
                "seek=1".to_string(),
            ];

            if let Err(e) = duct::cmd("dd", &cmdArgs).stdout_null().stderr_null().run() {
                eprintln!("ImageScript setBootloader: failed to run dd: {}", e);
                process::exit(1);
            }

            unsafe {
                BOOTLOADER = Some(bootloaderPath.to_string());
            }
        }
        _ => {
            eprintln!("ImageScript: Unknown operation {}", subOperation);
            process::exit(1);
        }
    }
}
