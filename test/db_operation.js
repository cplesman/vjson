const db = require('./db');
const bcrypt = require('bcryptjs');

async function main() {
  const arg1 = process.argv[2];

  if (!arg1) {
    console.error('Usage: node db_operation.js [read|append|update] <args...>');
    process.exit(1);
  }

  const validOperations = ['read', 'append', 'update'];
  if (!validOperations.includes(arg1.toLowerCase())) {
    console.error(`Invalid operation: ${arg1}. Valid operations are: ${validOperations.join(', ')}`);
    db.Close();
    process.exit(1);
  }
  const operation = arg1.toLowerCase();;
  if(operation==='read'){
    const objpath = process.argv[3];
    if(!objpath){
        console.error('Usage: node db_operation.js read <objpath>');
        db.Close();
        process.exit(1);
    }
    const result = db.Read(objpath);
    console.log(`Read result for path "${objpath}":`, JSON.stringify(result, null, 2));
  }

  db.Close();
}

main().catch(err => {
  console.error("Error during database operation:", err);
  db.Close();
  process.exit(1);
});