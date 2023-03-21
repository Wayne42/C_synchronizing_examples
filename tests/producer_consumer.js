const events = require('events');
const fs = require('fs');
const readline = require('readline');

function isAnagram(a, b) {
    var y = a.sort().join(""),
        z = b.sort().join("");
    return z === y;
}

(async function processLineByLine() {
    try {
        const rl = readline.createInterface({
            input: fs.createReadStream('test.txt'),
            crlfDelay: Infinity
        });

        let prod = [];
        let cons = [];

        rl.on('line', (line) => {
            // console.log(`Line: ${line}`);
            if (line.includes("Producer:")) {
                const keyword = "Wrote ";
                const len = "Wrote ".length;
                let i = line.indexOf(keyword) + len;

                prod.push(line[i]);
            } else if (line.includes("Consumer:")) {
                const keyword = "Consuming ";
                const len = "Consuming ".length;
                let i = line.indexOf(keyword) + len;

                cons.push(line[i]);
            }
        });

        await events.once(rl, 'close');

        // console.log(prod);
        // console.log(cons);

        if (isAnagram(prod, cons)) {
            console.log("external test success");
        } else {
            console.log("external test failed");
        }

        const used = process.memoryUsage().heapUsed / 1024 / 1024;
        console.log(`The testing script uses approximately ${Math.round(used * 100) / 100} MB`);
    } catch (err) {
        console.error(err);
    }
})();