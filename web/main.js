let device;
let encoder = new TextEncoder();
let decoder = new TextDecoder();
const endpoint = 2;
let passwordNames = [];

function send(what) {
    console.log("Sending: " + what);
    let encoded = encoder.encode(what);
    return device.transferOut(endpoint, encoded);
}

async function read() {
    const r = await device.transferIn(endpoint, 256);
    console.log("Read: " + decoder.decode(r.data));
    return decoder.decode(r.data);
}

async function connectDialog() {
    device = await navigator.usb.requestDevice({ filters: [{ vendorId: 0x30e0 }] })
    console.log("Device: " + device);
    await device.open();
    await device.selectConfiguration(1);
    await device.claimInterface(0);
    loadNames();
}

async function loadNames() {
    send('C');
    let count = await read();
    count = parseInt(count.split(":")[1]);
    console.log(count);
    passwordNames = [];

    $("#passwords-list").html("");
    for (let i = 0; i < count; i++) {
        await send(`N:${i}`);
        const name = await read();
        console.log("Reading: " + name);
        passwordNames[i] = name;
        const el = $(`<div class="box">
            <a href="#" onclick="showPassword(${i})">${name}</a>
            <a href="#" onclick="editPassword(${i})" style="float: right;">Aggiorna</a>
        </div>`);
        $("#passwords-list").append(el);
    }

    $("#add-password").removeClass("is-hidden");
    $("#connect-button").addClass("is-hidden");
}

async function readPassword(index) {
    await send(`R:${index}`);
    let password = await read();
    return password;
}

async function showPassword(index) {
    const name = passwordNames[index];
    const password = await readPassword(index);
    console.log(password);
    $("#password-modal .name").text(name);
    $("#password-modal .password").text(password);
    $("#password-modal").addClass("is-active");
}

function closePasswordModal() {
    $("#password-modal").removeClass("is-active");
    $("#edit-password-modal").removeClass("is-active");
    $("#new-password-modal").removeClass("is-active");
}

function editPassword(index) {
    const name = passwordNames[index];
    $("#edit-password-modal .name").text(name);
    $("#edit-password-modal .index").text(index);
    $("#edit-password-modal").addClass("is-active");
}

async function updatePassword() {
    const index = parseInt($("#edit-password-modal .index").text());
    const newPassword = $("#edit-password-field").val();
    console.log("Edit password " + index + newPassword);
    await send(`S:${index}:${newPassword}:`);
    closePasswordModal();
}

async function modalNewPassword() {
    $("#new-password-modal").addClass("is-active");
}

async function newPassword() {
    const name = $("#new-password-name").val();
    const password = $("#new-password-field").val();
    console.log("New password " + name + password);
    await send(`X:${name}:${password}:`);
    await loadNames();
    closePasswordModal();
}
