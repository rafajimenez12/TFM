using System;
using System.IO;
using System.Threading.Tasks;
using Microsoft.Azure.WebJobs;
using Microsoft.Azure.WebJobs.Extensions.Http;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using System.Security.Cryptography;
using System.Data;
using System.Linq;
using System.IdentityModel.Tokens.Jwt;
using JWT.Algorithms;
using JWT;
using JWT.Serializers;
using System.Collections.Generic;
using System.Data.SqlClient;

namespace Company.Function
{
    public class Login
    {


        private readonly IJwtAlgorithm _algorithm;
        private readonly IJsonSerializer _serializer;
        private readonly IBase64UrlEncoder _base64Encoder;
        private readonly IJwtEncoder _jwtEncoder;
        public Login()
        {
            // JWT specific initialization.
            // https://github.com/jwt-dotnet/jwt
            _algorithm = new HMACSHA256Algorithm();
            _serializer = new JsonNetSerializer();
            _base64Encoder = new JwtBase64UrlEncoder();
            _jwtEncoder = new JwtEncoder(_algorithm, _serializer, _base64Encoder);
        }
        [FunctionName("Login")]
        public static async Task<ResponseLogin> Run(
            [HttpTrigger(AuthorizationLevel.Function, "get", "post", Route = null)] HttpRequest req,
            ILogger log)
        {
            log.LogInformation("C# HTTP trigger function processed a request.");
            var str = Environment.GetEnvironmentVariable("SQL_CONNECTION_STRING");
            string name = req.Query["Nickname"];
            string psw = req.Query["Password"];
            string id ;
            string requestBody = await new StreamReader(req.Body).ReadToEndAsync();
            dynamic data = JsonConvert.DeserializeObject(requestBody);
            name = name ?? data?.Nickname;
            psw = psw ?? data?.Password;
             if (name == null || psw == null ){
                return new ResponseLogin { Status = "ERROR", Message = "Bad request" };
            }

            using (SqlConnection conn = new SqlConnection(str))

            {

                using (SqlCommand cmd = new SqlCommand())

                {

                    SqlDataReader dataReader;

                    cmd.CommandText = $"Select * from Users where Nickname = '{name}'";
                    cmd.CommandType = CommandType.Text;

                    cmd.Connection = conn;

                    conn.Open();

                    dataReader = cmd.ExecuteReader();
                    string password = req.Query["pswd"];
                    id = req.Query["userId"];
                    //string privilege = req.Query["privilege"];
                    //string subdepart = req.Query["subDepartment"];
                    var r = Serialize.SerializeData(dataReader).First();
                    id = r["userId"].ToString();

                    if (password != null)
                    {

                        /* Fetch the stored value */
                        //Encrypt Password to compare with the stored password
                        var savedPasswordHash = password;
                        /* Extract the bytes */
                        byte[] hashBytes = Convert.FromBase64String(savedPasswordHash);
                        /* Get the salt */
                        byte[] salt = new byte[16];
                        Array.Copy(hashBytes, 0, salt, 0, 16);
                        /* Compute the hash on the password the user entered */
                        var pbkdf2 = new Rfc2898DeriveBytes(psw, salt, 100000);
                        byte[] hash = pbkdf2.GetBytes(20);
                        /* Compare the results */
                        for (int i = 0; i < 20; i++)
                            if (hashBytes[i + 16] != hash[i])
                                //The password is incorrect
                                return new ResponseLogin { Status = "ERROR", Message = "Invalid User" };
                        ///////////////////////////////////// LOGIN CORRECTO//////////////////////////////////////


                    }
                    conn.Close();
                }
            }
            
            DateTime date = new DateTime();
            date = DateTime.Today.AddDays(1).ToUniversalTime();
            Int32 unixTimestamp = (int)date.Subtract(new DateTime(1970, 1, 1)).TotalSeconds;
            Dictionary<string, object> claims = new Dictionary<string, object>
            
        {
            // JSON representation of the user Reference with ID and display name
            { "userId", id },
            { "exp", unixTimestamp },
 
        };
            var _algorithm = new HMACSHA256Algorithm();
            var _serializer = new JsonNetSerializer();
             var _base64Encoder = new JwtBase64UrlEncoder();
             var _jwtEncoder = new JwtEncoder(_algorithm, _serializer, _base64Encoder);
            string token = _jwtEncoder.Encode(claims, "YOUR_SECRETY_KEY_JUST_A_LONG_STRING"); // Put this key in config
            return new ResponseLogin { Status = "OK", Message = "User loged", access_token = token,idUser = id };
        }
        public class ResponseLogin 
        {
            public string Status { set; get; }
            public string Message { set; get; }

            public string access_token { set; get; }

            public string idUser { set; get; }
        }
        
    }
}
