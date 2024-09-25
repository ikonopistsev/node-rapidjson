module.exports = class FNV1a {
    constructor(salt = 0x811c9dc5) {
        this.salt = salt;
    }

    hash(input) {
        let hval = this.salt;
        for (let i = 0; i < input.length; i++) {
            hval ^= input.charCodeAt(i);
            hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
        }
        return hval >>> 0; // Ensure unsigned 32-bit result
    }
}
