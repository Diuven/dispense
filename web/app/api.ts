class APIClient {
  API_URL = "http://localhost:9000";
  async fetchUser() {
    // const response = await fetch("/api/user");
    // set no cors
    // const response = await fetch(this.API_URL + "/hello/diuven");
    console.log("fetching user");
    const response = await fetch(this.API_URL + "/hello/diuven", {
      mode: "no-cors",
    });
    const data = await response.body;
    console.log(data);
    console.log("returned");
    return response.json();
  }
}

export default APIClient;
