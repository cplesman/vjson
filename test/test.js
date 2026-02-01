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

  const hashedPassword = await bcrypt.hash(password, 8);

  // Check if user already exists
    try{
        const existing = db.Find('/users', `"email" == "${email}"`);
        if (existing && Object.keys(existing).length > 0) {
            console.log(`User already exists for email: ${email} id=${Object.keys(existing)[0]}`);
            if(!existing[Object.keys(existing)[0]].isAdmin){
                console.log("However, the existing user is not an admin. Updating to have admin privileges.");
                if(!db.Update('/users/' + Object.keys(existing)[0], 'isAdmin', true)){
                    console.error("Failed to update user to admin.");
                }
            }
            console.log("Updating password");
            if(!db.Update('/users/' + Object.keys(existing)[0], 'password', hashedPassword)){
                console.error("Failed to update user password.");
            }
            // existing[Object.keys(existing)[0]].password = hashedPassword;
            // existing[Object.keys(existing)[0]].isAdmin = true;
            // db.Update('/users', Object.keys(existing)[0], existing[Object.keys(existing)[0]]);
            console.log(JSON.stringify(db.Read('/users/' + Object.keys(existing)[0]), null, 2));
            //db.Flush();
            db.Close();
            process.exit(1);
        }
    }catch(err){
        db.Append('/', "users", {}); //create empty users db
        console.log(err.message);
        console.log("Created users database.");
        //no users db yet
    }

  const user = {
    displayName: name,
    email,
    //password: hashedPassword, //for testing purposes run this js file again to add password
    isAdmin: true,
    emailVerified: true,
    tokens: [],
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

