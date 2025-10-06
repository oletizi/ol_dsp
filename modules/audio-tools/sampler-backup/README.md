# @oletizi/sampler-backup

**Automated, incremental backups for Akai hardware samplers via rsnapshot.**

## Purpose

The `sampler-backup` package provides reliable, space-efficient backups for vintage Akai hardware samplers. It wraps rsnapshot to create automated incremental backups from remote devices (via SSH/PiSCSI) or local media (SD cards, USB drives), ensuring decades of sampled instruments are preserved safely.

## Philosophy

**Incremental preservation with minimal space.** Rather than copying entire disk images repeatedly, this library provides:

- **Hard-linked snapshots**: Multiple backup intervals share unchanged files automatically
- **Intelligent rotation**: Same-day resume logic prevents duplicate backups
- **Remote-first design**: SSH-based backup from hardware via PiSCSI or similar SCSI emulators
- **Local media support**: Direct backup from SD cards and USB drives for floppy/SCSI emulators
- **Configurable retention**: Daily, weekly, and monthly intervals with customizable retention policies

## Local Media Support

**Version 1.0 includes local media backup!** You can now backup disk images directly from SD cards, USB drives, and other mounted storage media.

### Quick Start - Local Media

**1. Insert SD card or USB drive**

Your media will auto-mount:
- macOS: `/Volumes/SDCARD`
- Linux: `/media/$USER/SDCARD`

**2. Backup disk images**

```bash
akai-backup --source /Volumes/SDCARD
```

**3. Done!**

Disk images are incrementally backed up to `~/.audiotools/backup/daily.0/sdcard/`

### Use Cases

**Gotek Floppy Emulator:**
```bash
# Remove SD card from Gotek, insert into computer
akai-backup --source /Volumes/GOTEK --subdir gotek-s3000xl
```

**ZuluSCSI SCSI Emulator:**
```bash
# Remove SD card from ZuluSCSI, insert into computer
akai-backup --source /Volumes/ZULUSCSI --subdir zulu-s5000
```

**USB Drive:**
```bash
# Direct backup from any USB drive with disk images
akai-backup --source /Volumes/USB_DRIVE --subdir my-backups
```

### Features

- **Auto-detection**: Finds `.hds`, `.img`, and `.iso` files automatically
- **Incremental**: Skip unchanged files (same as remote backups)
- **Intervals**: Daily, weekly, monthly (same retention as rsnapshot)
- **Cross-platform**: macOS and Linux supported

For comprehensive documentation, see:
- **[Local Media User Guide](./docs/1.0.0/local-media-support/user-guide.md)**
- **[Platform Compatibility](./docs/1.0.0/local-media-support/platform-compatibility.md)**

## Design Approach

### rsnapshot Integration

Built on proven rsnapshot technology rather than reimplementing backup rotation:

- **Hard-linking filesystem snapshots**: rsync with `--link-dest` shares unchanged files across backups
- **Configurable intervals**: Traditional daily/weekly/monthly rotation or custom schedules
- **rsnapshot.conf generation**: Creates optimized configurations for sampler backups
- **Preserves rsnapshot semantics**: Users familiar with rsnapshot can customize directly

### Smart Rotation Logic

Backups are expensive (time, disk I/O, network transfer). The rotation strategy optimizes for:

- **Same-day resume**: If backup interrupted, resume rather than rotate intervals
- **Timestamp-based detection**: Uses mtime to identify today's backups
- **Graceful degradation**: Falls back to standard rotation if detection fails
- **Manual override**: Force new rotation with `--force-rotate` flag

### Dual-Mode Backup

Supports both traditional SSH-based remote backup and modern local media workflows:

```
┌─────────────────┐      ┌──────────────────┐
│ SSH/PiSCSI      │      │ Local Media      │
│                 │      │                  │
│ S5000 → PiSCSI  │      │ S3000 → SD Card  │
│     ↓  SSH      │      │     ↓  USB       │
│ backup host     │      │ backup host      │
└─────────────────┘      └──────────────────┘
        │                         │
        └─────────┬───────────────┘
                  ↓
         rsnapshot rotation
    (daily.0, weekly.0, monthly.0)
```

## Architecture

```
┌─────────────────────────┐
│ akai-backup batch       │  CLI orchestration
└───────────┬─────────────┘
            │
     ┌──────▼───────┐
     │ RsnaphotConfigGenerator │  Generate rsnapshot.conf
     └──────┬───────┘
            │
     ┌──────▼───────┐
     │ RsnaphotWrapper │  Execute rsnapshot with rotation logic
     │  - Same-day resume
     │  - Timestamp detection
     │  - Interval selection
     └──────┬───────┘
            │
     ┌──────▼───────┐
     │  rsnapshot    │  System rsnapshot binary
     │  - rsync      │  (installed via Homebrew/apt)
     │  - hard links │
     └───────────────┘
```

## Version 1.0

Version 1.0 provides rsnapshot integration with intelligent same-day resume and dual-mode backup support.

**Key Features:**
- Generate rsnapshot.conf for Akai sampler backups
- SSH-based remote backup via PiSCSI
- Local media backup (SD cards, USB drives)
- Smart same-day resume (no duplicate daily backups)
- Configurable retention (7 daily, 4 weekly, 12 monthly by default)
- Batch processing for multiple samplers

**Supported Platforms:**
- macOS (darwin-arm64, darwin-x64)
- Linux (x64, ARM64)
- Windows via WSL2

**Documentation:**

- 📦 [Installation Guide](./docs/1.0/installation.md)
- 🚀 [Quick Start](./docs/1.0/quick-start.md)
- 📟 [CLI Commands](./docs/1.0/cli-commands.md)
- ⚙️ [Configuration](./docs/1.0/configuration.md)
- 💡 [Examples](./docs/1.0/examples.md)
- 📚 [API Reference](./docs/1.0/api-reference.md)
- 🔧 [Troubleshooting](./docs/1.0/troubleshooting.md)
- 📖 [Complete v1.0 Documentation](./docs/1.0/README.md)

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](../../CONTRIBUTING.md) for development guidelines.

**Development Setup:**
```bash
git clone https://github.com/yourusername/audio-tools.git
cd audio-tools/sampler-backup
pnpm install
pnpm run build
pnpm test
```

## License

Apache-2.0

## Credits

**Author:** Orion Letizi

**Related Projects:**
- [@oletizi/sampler-export](https://www.npmjs.com/package/@oletizi/sampler-export) - Disk image extraction and format conversion
- [@oletizi/sampler-devices](https://www.npmjs.com/package/@oletizi/sampler-devices) - Akai device abstraction

**External Dependencies:**
- [rsnapshot](https://rsnapshot.org/) - Filesystem snapshot utility
- [rsync](https://rsync.samba.org/) - Incremental file transfer

---

**Need Help?**

- 📖 [Documentation](./docs/1.0/README.md)
- 🐛 [Report Issues](https://github.com/yourusername/audio-tools/issues)
- 💬 [Discussions](https://github.com/yourusername/audio-tools/discussions)
