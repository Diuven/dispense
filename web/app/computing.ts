"use client";

import React, { useState, useCallback, useEffect } from "react";
import useWebSocket, { ReadyState } from "react-use-websocket";

const SERVER_URL = "ws://localhost:9000";

class Vector {
  size: number;
  data: number[];

  constructor() {
    this.size = 0;
    this.data = [];
  }

  deserialize(str: string) {
    this.data = str
      .split(" ")
      .map((x) => parseInt(x))
      .filter((x) => !isNaN(x));
    this.size = this.data.length;
  }

  dot(other: Vector) {
    let result = 0;
    for (let i = 0; i < this.size; i++) {
      result += this.data[i] * other.data[i];
    }
    return result;
  }
}

function splitString(str: string, delimiter: string = ";;") {
  return str.split(delimiter);
}
function splitMessage(str: string) {
  let [node_id_str, op_type, action_id_str, data] = splitString(str);
  let node_id = parseInt(node_id_str);
  let action_id = parseInt(action_id_str);
  return { node_id, op_type, action_id, data };
}
function formatMessage(
  node_id: number,
  op_type: string,
  action_id: number,
  data: string
) {
  return `${node_id};;${op_type};;${action_id};;${data}`;
}

export class NodeHandler {
  stop = false;
  node_id: number;
  action_id: number;
  a: Vector;
  b: Vector;

  constructor(node_id: number) {
    this.node_id = node_id;
    this.action_id = -1;
    this.a = new Vector();
    this.b = new Vector();
  }

  makeEnterMessage() {
    return formatMessage(this.node_id, "ENTER", -1, "");
  }

  handleStop(data: string) {
    console.log("Received stop message. Stopping...");
    this.stop = true;
    return ["", ""];
  }

  async handleEnterResp(got_action_id: number, data: string) {
    let success = parseInt(data);
    if (success) {
      this.action_id = got_action_id;
      console.log("Successfully entered the network");
      await new Promise((resolve) => setTimeout(resolve, 1000));
      return ["NUDGE", ""];
    } else {
      console.log(
        "Failed to enter the network. Possibly id collision. Exiting..."
      );
      this.stop = true;
      return ["", ""];
    }
  }

  async handleNudgeResp(data: string) {
    let success = parseInt(data);
    if (success) {
      console.log("Still in the network. Waiting...");
      await new Promise((resolve) => setTimeout(resolve, 1000));
      return ["NUDGE", ""];
    } else {
      console.log("Should not happen. Exiting...");
      this.stop = true;
      return ["", ""];
    }
  }

  handleAssignAction(got_action_id: number, data: string) {
    this.action_id = got_action_id;
    this.a = new Vector();
    this.b = new Vector();
    console.log("Assigned action: " + this.action_id);
    return ["GET_A", ""];
  }

  handleGetAResp(got_action_id: number, data: string) {
    if (got_action_id != this.action_id) {
      console.log("Action ID mismatch in handle_get_a_resp. Exiting...");
      this.stop = true;
      return ["", ""];
    }

    this.a.deserialize(data);
    console.log("Received vector A with size: " + this.a.size);
    return ["GET_B", ""];
  }

  handleGetBResp(got_action_id: number, data: string) {
    if (got_action_id != this.action_id) {
      console.log("Action ID mismatch in handle_get_b_resp. Exiting...");
      this.stop = true;
      return ["", ""];
    }

    this.b.deserialize(data);

    let result = this.a.dot(this.b);
    return ["RETURN", result.toString()];
  }

  async messageHandler(message: string) {
    let { node_id, op_type, action_id, data } = splitMessage(message);
    console.log(
      `Received operation: ${op_type} of action id ${this.action_id} with data length of : ${data.length}`
    );

    let res_op_type = "";
    let res_data = "";

    if (op_type == "STOP") [res_op_type, res_data] = this.handleStop(data);
    else if (op_type == "ENTER_RESP")
      [res_op_type, res_data] = await this.handleEnterResp(action_id, data);
    else if (op_type == "NUDGE_RESP")
      [res_op_type, res_data] = await this.handleNudgeResp(data);
    else if (op_type == "ASSIGN_ACTION")
      [res_op_type, res_data] = this.handleAssignAction(action_id, data);
    else if (op_type == "GET_A_RESP")
      [res_op_type, res_data] = this.handleGetAResp(action_id, data);
    else if (op_type == "GET_B_RESP")
      [res_op_type, res_data] = this.handleGetBResp(action_id, data);
    else {
      console.log(`Invalid operation ${op_type} with data: ${data}`);
    }

    if (action_id > 0 && action_id != this.action_id) {
      console.log("Action ID mismatch in message_handler. Exiting...");
      this.stop = true;
    }

    if (res_op_type == "") return "";
    else {
      let response = formatMessage(
        node_id,
        res_op_type,
        this.action_id,
        res_data
      );
      return response;
    }
  }
}

export class WebsocketManager {
  socketUrl: string;
  messageHistory: MessageEvent<any>[];
  sendMessage: (message: string) => void;
  lastMessage: MessageEvent<any> | null;
  readyState: ReadyState;

  constructor() {
    this.socketUrl = SERVER_URL;
    this.messageHistory = [];
    this.sendMessage = () => {};
    this.lastMessage = null;
    this.readyState = ReadyState.UNINSTANTIATED;
  }

  async start() {
    const { sendMessage, lastMessage, readyState } = useWebSocket(
      this.socketUrl
    );
    this.sendMessage = sendMessage;
    this.lastMessage = lastMessage;
    this.readyState = readyState;
  }

  async stop() {}
}

/*
export default function Page() {
  const [socketUrl, setSocketUrl] = useState(SERVER_URL);
  const [messageHistory, setMessageHistory] = useState<MessageEvent<any>[]>([]);

  const { sendMessage, lastMessage, readyState } = useWebSocket(socketUrl);

  useEffect(() => {
    if (lastMessage !== null) {
      setMessageHistory((prev) => prev.concat(lastMessage));
    }
  }, [lastMessage]);

  const handleClickChangeSocketUrl = useCallback(
    () => setSocketUrl("wss://demos.kaazing.com/echo"),
    []
  );

  const handleClickSendMessage = useCallback(() => sendMessage("Hello"), []);

  const connectionStatus = {
    [ReadyState.CONNECTING]: "Connecting",
    [ReadyState.OPEN]: "Open",
    [ReadyState.CLOSING]: "Closing",
    [ReadyState.CLOSED]: "Closed",
    [ReadyState.UNINSTANTIATED]: "Uninstantiated",
  }[readyState];

  //   return (
  //     <div>
  //       <button onClick={handleClickChangeSocketUrl}>
  //         Click Me to change Socket Url
  //       </button>
  //       <button
  //         onClick={handleClickSendMessage}
  //         disabled={readyState !== ReadyState.OPEN}
  //       >
  //         Click Me to send 'Hello'
  //       </button>
  //       <span>The WebSocket is currently {connectionStatus}</span>
  //       {lastMessage ? <span>Last message: {lastMessage.data}</span> : null}
  //       <ul>
  //         {messageHistory.map((message, idx) => (
  //           <span key={idx}>{message ? message.data : null}</span>
  //         ))}
  //       </ul>
  //     </div>
  //   );
}
*/
