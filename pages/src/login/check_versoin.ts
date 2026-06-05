export async function getLatestVersion() {
  const response = await fetch(
    "https://api.github.com/repos/liehuoe/WeiXins/releases/latest",
  );
  if (!response.ok) {
    throw new Error(`HTTP 错误！状态码: ${response.status}`);
  }
  const data = await response.json();
  return data.tag_name.replace("v", "");
}
