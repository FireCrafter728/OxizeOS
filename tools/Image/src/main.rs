#![allow(non_snake_case)]
#![allow(static_mut_refs)]

use serde::Deserialize; 
use std::env;
use std::fs::{self};
use std::process;

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
    let mut bps : Option<String> = None;
    let mut count = None;

    for pair in flags.chunks(2) {
        if let [keyVal, valVal] = pair {
            match(keyVal.as_str(), valVal.as_str()) {
                (Some("input"), Some(v)) => input = Some(v.to_string()),
                (Some("output"), Some(v)) => output = Some(v.replace("$ARG1", arg1)),
                _ => {
                    if keyVal == "bps" {
                        if let Some(s) = valVal.as_str() {
                            bps = Some(s.to_string());
                        } else if let Some(n) = valVal.as_u64() {
                            bps = Some(n.to_string());
                        }
                        
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
    cmdArgs.push(format!("bs={}", bps.as_deref().unwrap_or("512")));
    cmdArgs.push(format!("count={}", count.unwrap_or(2880)));

    println!("Creating disk {}, sectors: {}, bps: {}", output, count.unwrap_or(2880), bps.as_deref().unwrap_or("512"));

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
    let mut CmdArgs = vec![];
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

    match subOperation.as_str() {
        "mkgpt" => {
            CmdArgs.push("--zap-all".to_string());
            CmdArgs.push("-o".to_string());
        }
        "mkpart" => {
            let mut partIndex: Option<u16> = Some(1);
            let mut partLabel: Option<String> = Some(String::new());
            let mut partType: Option<String> = Some("0000".to_string());
            let mut startLba = None;
            let mut sectorCount = None;
            
            if let Some(extra) = extraflags {
                for pair in extra.chunks(2) {
                    if let [key, val] = pair {
                        match key.as_str() {
                            Some("partIndex") => {
                                if let Some(n) = val.as_u64() {
                                    partIndex = Some(n as u16);
                                }
                            }
                            Some("partLabel") => {
                                partLabel = val.as_str().map(|s| s.to_string());
                            }
                            Some("partType") => {
                                partType = val.as_str().map(|s| s.to_string());
                            }
                            Some("partStartLba") => {
                                if let Some(n) = val.as_u64() {
                                    startLba = Some(n);
                                }
                            }
                            Some("partSectors") => {
                                if let Some(n) = val.as_u64() {
                                    sectorCount = Some(n);
                                }
                            }
                            _ => {}
                        }
                    }
                }
            }

            if let (Some(index), Some(label), Some(ptype), Some(start), Some(size)) = (partIndex, partLabel, partType, startLba, sectorCount) {
                let end = start + size - 1;

                CmdArgs.push("-n".to_string());
                CmdArgs.push(format!("{}:{}:{}", index, start, end));
                CmdArgs.push("-t".to_string());
                CmdArgs.push(format!("{}:{}", index, ptype));
                CmdArgs.push("-c".to_string());
                CmdArgs.push(format!("{}:{}", index, label));
                println!("Creating partition {} on disk {}, label: {}, part-type: {}, start LBA: {}, sectors: {}", index, inputFile, label, ptype, start, size);
            } else {
                eprintln!("DiskPart: subOperation mkpart requires partition index, partition label, type, partition start LBA & partition size in sectors");
                process::exit(1);
            }
        }
        _ => {
            eprintln!("DiskPart: Unknown operation {}", subOperation);
            process::exit(1);
        }
    }

    CmdArgs.push(inputFile.clone());

    if let Err(e) = duct::cmd("sgdisk", CmdArgs).stdout_null().run() {
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

    if let Err(e) = duct::cmd("output/fat", CmdArgs).run() {
        eprintln!("Failed to run FAT: {}", e);
        process::exit(1);
    }
}
