const spawnSync = require('child_process').spawnSync;
const exec = cmd => {
    const { status } = spawnSync(cmd, {
        shell: true,
        stdio: 'inherit',
    });
    process.exit(status);
}

exec("npx cmake-js rebuild --config Release");
