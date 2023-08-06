import { ESPLoader, FlashOptions, IEspLoaderTerminal, LoaderOptions, Transport } from "esptool-js"


let device: SerialPort | null = null;
let transport: Transport;
let chip: string | null = null;
let esploader: ESPLoader;
let terminal: IEspLoaderTerminal;

let connectButton: HTMLButtonElement;
let flashButton: HTMLButtonElement;
let releaseList: HTMLUListElement;

let releases: Release[] = []
let fileArray: any[] = []

const manifestUrl = "/api/releases.json";

document.addEventListener("DOMContentLoaded", function () {
    terminal = createTerminal(document.getElementById("console")!)

    connectButton = document.getElementById("connect") as HTMLButtonElement;
    connectButton.onclick = connect;

    flashButton = document.getElementById("flash") as HTMLButtonElement;
    flashButton.onclick = flashFull;

    releaseList = document.getElementById("release-list") as HTMLUListElement;

    updateUi();

    loadReleases();
});

function updateUi() {
    connectButton.disabled = chip != null;

    flashButton.disabled = chip == null || fileArray.length == 0;
}

function createTerminal(element: HTMLElement): IEspLoaderTerminal {
    return {
        clean() {
            element.innerText = "";
            element.scrollTop = element.scrollHeight;
        },
        writeLine(data: string) {
            console.log(data);
            element.innerText = element.innerText.concat(data + "\n");
            element.scrollTop = element.scrollHeight;
        },
        write(data: string) {
            console.log(data);
            element.innerText = element.innerText.concat(data);
            element.scrollTop = element.scrollHeight;
        },
    };
}

async function connect(): Promise<void> {
    connectButton.disabled = true;

    if (device === null) {
        device = await navigator.serial.requestPort();
        transport = new Transport(device);
    }

    try {
        const options: LoaderOptions = {
            transport,
            baudrate: 460800,
            romBaudrate: 460800,
            terminal: terminal,
        };
        esploader = new ESPLoader(options);

        chip = await esploader.main_fn();
    } catch (e) {
        console.error(e);
    } finally {
        updateUi();
    }
};

async function flashFull(): Promise<void> {
    try {
        const flashOptions: FlashOptions = {
            fileArray,
            flashSize: "keep",
            eraseAll: true,
            compress: true,
            reportProgress: (fileIndex, written, total) => {
                console.log(`Flash progress: ${(written / total) * 100}`);
            },
        } as FlashOptions;
        await esploader.write_flash(flashOptions);
    } catch (e: any) {
        console.error(e);
        terminal.writeLine(`Error: ${e.message}`);
    }
}

async function loadReleases() {
    const response = await fetch(manifestUrl);
    releases = await response.json() as Release[];

    releases.forEach((release, index, _arr) => {
        const li = document.createElement("li");
        li.innerText = release.name;
        li.onclick = () => { selectRelease(index); };
        releaseList.appendChild(li);
    });
}

async function selectRelease(index: number): Promise<void> {
    const release = releases[index];

    console.log(`Downloading release ${release.name}`);
    for (const partition of release.partitions) {
        console.log(`Downloading ${partition.name}`);

        const response = await fetch(partition.url);

        const bstr = await blobToBinaryString(await response.blob());
        fileArray.push({
            data: bstr,
            address: Number(partition.offset),
        });
    }
    console.log("Download done");

    updateUi();
}

// Debug only
async function blobToBase64(blob: Blob): Promise<string> {
    return new Promise((resolve, _) => {
        const reader = new FileReader();
        reader.onloadend = () => {
            // Assume Chrome ~115
            const res = reader.result as string

            // Remove data:application/octet-stream;base64,
            const b64 = res.substring(res.indexOf(',') + 1);
            resolve(b64);
        }
        reader.readAsDataURL(blob);
    });
}

async function blobToBinaryString(blob: Blob): Promise<string> {
    return new Promise((resolve, _) => {
        const reader = new FileReader();
        reader.onloadend = () => {
            resolve(reader.result as string);
        }
        reader.readAsBinaryString(blob);
    });
}
