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
        eprintln!("\x1b[31mSyntax: {} <image.json> <disk image>\x1b[0m", args[0]);
        std::process::exit(1);
    }

    let filename = &args[1];
    let arg1 = &args[2];

    let file_content = fs::read_to_string(filename).expect("\x1b[31mFailed to read file\x1b[0m");
    let config: ImageConfig = serde_json::from_str(&file_content).expect("\x1b[31mInvalid JSON format\x1b[0m");

    println!("\x1b[1m\x1b[33m[IMAGE] \x1b[91mBuilding image:\x1b[0m");

    for (index, entry) in config.entries.iter().enumerate() {
        match entry.operation.as_str() {
            "createdisk" => CreateDisk(&entry.flags, arg1),
            "diskpart" => DiskPart(&entry.flags, &entry.extraflags, arg1),
            "fat" => Fat(&entry.flags, &entry.extraflags, arg1),
            other => {
                eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mEntry {}: Unknown operation {}\x1b[0m", index + 1, other);
                process::exit(1);
            }
        }
    }
}

fn CreateDisk(flags: &[serde_json::Value], arg1: &str) {
    let mut cmdArgs = vec![];
    let mut output = None;
    let mut size : Option<String> = None;
    let mut format : Option<String> = None;

    for pair in flags.chunks(2) {
        if let [keyVal, valVal] = pair {
            match(keyVal.as_str(), valVal.as_str()) {
                (Some("output"), Some(v)) => output = Some(v.replace("$ARG1", arg1)),
                _ => {
                    if keyVal == "size" {
                        if let Some(s) = valVal.as_str() {
                            size = Some(s.to_string());
                        }
                    } else if keyVal == "format" {
                        if let Some(s) = valVal.as_str() {
                            format = Some(s.to_string());
                        }
                    }
                }
            }
        } else {
            eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mMalformed flags pair : {:?}\x1b[0m", pair);
            process::exit(1);
        }
    }

    let output = output.unwrap_or_else(|| {
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mCreateDisk: missing output file\x1b[0m");
        process::exit(1);
    });

    let size = size.unwrap_or_else(|| {
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mCreateDisk: missing size\x1b[0m");
        process::exit(1);
    });
    
    cmdArgs.push("create");
    cmdArgs.push("-f");
    cmdArgs.push(format.as_deref().unwrap_or("raw"));
    cmdArgs.push(&output);
    cmdArgs.push(&size);

    println!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[96mCreating disk of size {}\x1b[0m", size);

    if let Err(e) = duct::cmd("qemu-img", cmdArgs).stdout_null().stderr_null().run() {
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFailed to create DISK {}\x1b[0m", e);
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
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mDiskPart: missing subOperation\x1b[0m");
        process::exit(1);
    });

    let inputFile = inputFile.unwrap_or_else(|| {
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mDiskPart: missing input file\x1b[0m");
        process::exit(1);
    });

    CmdArgs.push(inputFile.clone());

    match subOperation.as_str() {
        "mkgpt" => {
            CmdArgs.push("MKGPT".to_string());
            println!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[96mCreating GPT Structures\x1b[0m")
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

                CmdArgs.push("MKPART".to_string());
                CmdArgs.push(label.to_string());
                CmdArgs.push(ptype.to_string());
                CmdArgs.push(start.to_string());
                CmdArgs.push(end.to_string());
                CmdArgs.push(index.to_string());
                println!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[96mCreating partition \"{}\" with partition type {}, start LBA {}, sectors {} and partition index {}\x1b[0m", label, ptype, start, size, index);
            } else {
                eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mDiskPart: subOperation mkpart requires partition index, partition label, type, partition start LBA & partition size in sectors\x1b[0m");
                process::exit(1);
            }
        }
        _ => {
            eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mDiskPart: Unknown operation {}\x1b[0m", subOperation);
            process::exit(1);
        }
    };

    if let Err(e) = duct::cmd("output/gpt", CmdArgs).stdout_null().run() {
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFailed to partition DISK: {}\x1b[0m", e);
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
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFAT: Missing input file\x1b[0m");
        process::exit(1);
    });

    CmdArgs.push(inputFile.clone());

    let subOperation = subOperation.unwrap_or_else(|| {
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFAT: missing subOperation\x1b[0m");
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
                    println!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[92mCreating File / Directory on disk at path \"{}\"\x1b[0m", extra[1].as_str().unwrap_or("").to_string());
                } else {
                    eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFAT: Not enough extraflags for {}\x1b[0m", subOperation);   
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
                    println!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[92mCopying file to disk at path \"{}\", with path in disk \"{}\"\x1b[0m", extra[1].as_str().unwrap_or("").to_string(), extra[3].as_str().unwrap_or("").to_string());
                } else {
                    eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFAT: Not enough extraflags for diskcpy\x1b[0m");
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
                                eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFAT: Unknown mkfs extraflag {}\x1b[0m", k);
                                process::exit(1);
                            }
                        }
                    }
                }
                println!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[92mFormatting disk\x1b[0m");
            }
        }
        _ => {
            eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFAT: Unknown operation {}\x1b[0m", subOperation);
            process::exit(1);
        }
    }

    if let Err(e) = duct::cmd("output/fat", CmdArgs).run() {
        eprintln!("    \x1b[1m\x1b[33m[IMAGE]  \x1b[31mFailed to run FAT: {}\x1b[0m", e);
        process::exit(1);
    }
}