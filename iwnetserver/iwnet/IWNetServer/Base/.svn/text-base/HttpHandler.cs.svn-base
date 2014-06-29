using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Linq;
using Kayak;
using System.Threading.Tasks;
using System.Threading;
using System.Xml.Linq;

namespace IWNetServer
{
    public delegate void
        OwinApplication(IDictionary<string, object> env,
        Action<string, IDictionary<string, IList<string>>, IEnumerable<object>> respond,
        Action<Exception> error);

    class HttpHandler
    {
        public static void Run()
        {
            var server = new DotNetServer();

            var pipe = server.Start();

            server.Host((env, respond, error) =>
                {
                    var path = env["Owin.RequestUri"] as string;
                    /*
                    if (path == "/")
                        respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/plain" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes("Hello world.") }
                            );
                    else if (path == "/stream")
                    {
                        StreamingApp(env, respond, error);
                    }
                    else if (path == "/bufferedecho")
                    {
                        BufferedEcho(env, respond, error);
                    }
                    */

                    var urlParts = path.Substring(1).Split(new[] { '/' }, 2);

                    if (urlParts.Length >= 1)
                    {
                        if (urlParts[0] == "nick")
                        {
                            var steamID = long.Parse(urlParts[1]);
                            var client = Client.Get(steamID);

                            if (client.GameVersion != 0)
                            {
                                var nickname = client.GamerTag;
                                respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/plain" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes(nickname) }
                                );
                                return;
                            }
                        }
                        else if (urlParts[0] == "stats")
                        {
                            var statistics = Client.GetStatistics();
                            var retString = "[Stats]\r\n";
                            var totalPlayers = 0;

                            foreach (var stat in statistics)
                            {
                                retString += string.Format("playerState{0}={1}\r\n", stat.StatisticID, stat.StatisticCount);

                                totalPlayers += stat.StatisticCount;
                            }

                            retString += string.Format("totalPlayers={0}\r\n", totalPlayers);

                            retString += "[Lobbies]\r\n";
                            totalPlayers = 0;
                            foreach (var serverc in MatchServer.Servers)
                            {
                                var cnt = serverc.Value.Sessions.Count(sess => (DateTime.Now - sess.LastTouched).TotalSeconds < 60);
                                //var cnt = serverc.Value.Sessions.Count;
                                retString += string.Format("lobbies{0}={1}\r\n", serverc.Key, cnt);
                                totalPlayers += cnt;
                            }

                            retString += string.Format("totalLobbies={0}", totalPlayers);
                            respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/plain" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes(retString) }
                                );
                            return;
                        }
                        else if (urlParts[0] == "clean")
                        {
                            long clientID = 0;
                            var valid = false;
                            
                            if (long.TryParse(urlParts[1], out clientID))
                            {
                                if (clientID != 0x110000100000002)
                                {
                                    var client = Client.Get(clientID);

                                    if ((DateTime.UtcNow - client.LastTouched).TotalSeconds < 300)
                                    {
                                        var state = CIServer.IsUnclean(clientID, null);

                                        if (!state)
                                        {
                                            valid = true;
                                        }
                                    }
                                }
                            }
                            respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/plain" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes((valid) ? "valid" : "invalid") }
                                );
                            return;
                        }
                        else if (urlParts[0] == "cleanExt")
                        {
                            long clientID = 0;
                            var valid = false;
                            var reason = "invalid-id";

                            if (long.TryParse(urlParts[1], out clientID))
                            {
                                if (clientID != 0x110000100000002)
                                {
                                    var client = Client.Get(clientID);

                                    if ((DateTime.UtcNow - client.LastTouched).TotalHours < 6)
                                    {
                                        var state = CIServer.IsUnclean(clientID, null);

                                        if (state)
                                        {
                                            reason = CIServer.WhyUnclean(clientID);
                                        }
                                        else
                                        {
                                            reason = "actually-valid";
                                            valid = true;
                                        }
                                    }
                                    else
                                    {
                                        reason = "not-registered-at-lsp";
                                    }
                                }
                                else
                                {
                                    reason = "00002-IDGEN-error";
                                }
                            }
                            respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/plain" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes(((valid) ? "valid" : "invalid") + "\r\n" + reason) }
                                );

                            return;
                        }
                        else if (urlParts[0] == "pc")
                        {
                            var file = "pc/" + urlParts[1].Replace('\\', '/');

                            if (!File.Exists(file))
                            {
                                respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/plain" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes("Not found!") }
                                );
                            }
                            else
                            {
                                var stream = File.OpenRead(file);
                                var b = new byte[stream.Length];
                                stream.Read(b, 0, (int)stream.Length);
                                stream.Close();

                                respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "application/octet-stream" } }
                                },
                                new object[] { b }
                                );
                            }

                            return;
                        }
                        else if (urlParts[0] == "servers")
                        {
                            var filter = urlParts[1].Replace(".xml", "");
                            if (filter.Contains("yeQA4reD"))
                            {
                                respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/xml" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes(ProcessServers(filter)) }
                                );
                            }
                            else
                            {
                                respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/html" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes("Invalid") }
                                );
                            }
                            return;
                        }
                        else
                        {
                                respond(
                                "200 OK",
                                new Dictionary<string, IList<string>>() 
                                {
                                    { "Content-Type",  new string[] { "text/html" } }
                                },
                                new object[] { Encoding.ASCII.GetBytes("Invalid") }
                                );
                        }
                    }
                });
            Log.Info("Listening on " + server.ListenEndPoint);
        }

        private static string ProcessServers(string filter)
        {
            var data = ProcessQuery(filter);
            try
            {
                lock (ServerParser.Servers)
                {
                    var servers = from server in ServerParser.Servers
                                  where ((server.Value.Address != null) && (server.Value.GameData.Count > 0) && ((DateTime.UtcNow - server.Value.LastUpdated).TotalSeconds < 600) && FilterMatches(server.Value.GameData, data))
                                  select server;

                    var start = 0;
                    var limit = 50;
                    var sort = "sv_hostname";

                    if (data.ContainsKey("start"))
                    {
                        int.TryParse(data["start"], out start);
                    }

                    if (data.ContainsKey("count"))
                    {
                        int.TryParse(data["count"], out limit);
                    }

                    if (data.ContainsKey("sort"))
                    {
                        sort = data["sort"];
                    }

                    servers = servers.OrderBy(gdata => (gdata.Value.GameData.ContainsKey(sort)) ? gdata.Value.GameData[sort] : "");

                    // intervene to get full count
                    var count = servers.Count();

                    servers = servers.Skip(start);
                    servers = servers.Take(limit);

                    var elements = from server in servers
                                   select new XElement("Server",
                                                           new XElement("HostXUID", server.Value.HostXUID.ToString("X16")),
                                                           new XElement("HostName", (server.Value.HostName == null) ? "unknown" : server.Value.HostName),
                                                           new XElement("LastUpdated", server.Value.LastUpdated.ToString("s")),
                                                           new XElement("ConnectIP", server.Value.Address.ToString()),
                                                           new XElement("PlayerCount", server.Value.CurrentPlayers.ToString()),
                                                           new XElement("MaxPlayerCount", server.Value.MaxPlayers.ToString()),
                                                           new XElement("InGame", server.Value.InGame.ToString()),
                                                           server.Value.GetDataXElement());

                    var countAttribute = new XAttribute("Count", count);

                    var doc = new XDocument(new XElement("Servers", countAttribute, elements));
                    /*foreach (var element in elements)
                    {
                        rootElement.Add(element);
                    }*/

                    return doc.ToString();
                }

            }
            catch
            {
                return "<Servers Count=\"0\"></Servers>";
            }
        }
        private static bool FilterMatches(Dictionary<string, string> data, Dictionary<string, string> filter)
        {
            foreach (var entry in data)
            {
                if (filter.ContainsKey(entry.Key))
                {
                    if (filter[entry.Key] == "*")
                    {
                        if (entry.Value == "")
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (filter[entry.Key] != entry.Value)
                        {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        private static Dictionary<string, string> ProcessQuery(string query)
        {
            var items = query.Split('/');
            var retval = new Dictionary<string, string>();

            foreach (var item in items)
            {
                var data = item.Split('=');

                if (data.Length == 1)
                {
                    retval.Add(item, "on");
                }
                else
                {
                    retval.Add(data[0], data[1]);
                }
            }

            return retval;
        }
    }
}
