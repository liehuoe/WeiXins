export { EmptyText };

function EmptyText(props: { children: any }) {
  return (
    <div class="h-full items-center justify-center text-2xl text-gray-300">
      {props.children}
    </div>
  );
}
