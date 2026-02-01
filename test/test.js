const db = require('./db');
const bcrypt = require('bcryptjs');

async function main() {
  const arg1 = process.argv[2];
  const arg2 = process.argv[3];

  if (!arg1) {
    console.error('Usage: node init_admin.js <password> OR node init_admin.js <email> <password> [name]');
    process.exit(1);
  }

  // If two args provided: treat as <email> <password>
  // If only one arg: treat as <password> and use default email
  const email = arg2 ? arg1 : (process.env.ADMIN_EMAIL || 'admin@gridtowing.ca');
  const password = arg2 ? arg2 : arg1;
  const name = process.argv[4] || email.split('@')[0];

  // Check if user already exists
    try{
        const existing = db.Find('/users', `"email" == "${email}"`);
        if (existing && Object.keys(existing).length > 0) {
            console.error(`User already exists for email: ${email}`);
            process.exit(1);
        }
    }catch(err){
        db.Append('/', "users", {}); //create empty users db
        console.log(err.message);
        console.log("Created users database.");
        //no users db yet
    }

  const hashedPassword = await bcrypt.hash(password, 8);

  const user = {
    displayName: name,
    email,
    password: hashedPassword,
    isAdmin: true,
    emailVerified: true,
    tokens: [],
    createdAt: new Date().toISOString(),
    lastLogin: null
  };

  try{
    let id = db.Append('/users', user);
    console.log(`Admin user created: ${email} (${id})`);
  }catch(err){
    console.error("Error creating user:", err);
  }

  db.Close(); //flush and close database
}

main().catch((err) => {
  console.error('Failed to create admin user:', err);
  db.Close();
  process.exit(1);
});

