static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length)
  {
    if (pos < 124 * BLOCK_SECTOR_SIZE)
    {  
      int direct_num = pos / BLOCK_SECTOR_SIZE;
      return inode->data.direct[direct_num] + pos / BLOCK_SECTOR_SIZE;
    }
    else if (pos < (124 + 128) * BLOCK_SECTOR_SIZE)
    {
      int indirect_num = pos / BLOCK_SECTOR_SIZE - 124;
    }
  }
  else
    return -1;
}