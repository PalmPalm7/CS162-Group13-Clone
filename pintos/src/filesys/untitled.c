static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length)
  {
    if (pos < 124 * BLOCK_SECTOR_SIZE)
    {  
      int direct_num = pos / BLOCK_SECTOR_SIZE;
      return inode->data.direct[direct_num];
    }
    else if (pos < (124 + 128) * BLOCK_SECTOR_SIZE)
    {
      int indirect_num = pos / BLOCK_SECTOR_SIZE - 124;
      return inode->data->indirect + indirect_num; 
    }
    else 
    {
      int doubly_indirect_num = (pos / BLOCK_SECTOR_SIZE - 124 - 128) / 128;
      int inner_indirect_num = pos / BLOCK_SECTOR_SIZE - doubly_indirect_num;
      return inode->data->doubly_indirect[doubly_indirect_num][inner_indirect_num];
    }
  }
  else
    return -1;
}