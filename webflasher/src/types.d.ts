interface Partition {
    name: string;
    url: string;
    offset: string;
}

interface Release {
    name: string;
    partitions: Partition[];
}
